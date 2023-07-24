// Copyright (C) 2007 Id Software, Inc.
//


#include "Precompiled.h"
#pragma hdrstop

#include "CompiledScriptInterface.h"
#include "CompiledScript_Event.h"
#include "CompiledScript_Class.h"

sdCompilerInterface* compilerInterface = NULL;
bool initialised = false;

sdCSTypeInfo sdCompiledScript_ClassBase::Type( "sdCompiledScript_ClassBase", "" );

class sdCompiledScriptInterface : public sdScriptInterface {
public:
	virtual sdClassInfo*			GetClassInfo( void ) { return sdCompiledScript_Class::GetClassInfo(); }
	virtual sdFunctionInfo*			GetFunctionInfo( void ) { return sdCompiledScript_Class::GetFunctionInfo(); }

	virtual int						GetClassFunctionInfoSize( void ) { return sizeof( sdClassFunctionInfo ); }
	virtual int						GetFunctionInfoSize( void ) { return sizeof( sdFunctionInfo ); }

	virtual int						GetVersion( void ) { return COMPILED_SCRIPT_INTERFACE_VERSION; }

} g_compiledScriptInterface;

#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
extern "C" sdScriptInterface* InitScripts( sdCompilerInterface* iface ) {
	int size = sizeof( sdClassFunctionInfo );

	compilerInterface = iface;

	if ( !initialised ) {
		sdCSTypeInfo::Init();
		sdCompiledScript_Event::Startup();

		sdCompiledScript_Class::InitClasses();
		sdCompiledScript_Class::InitFunctions();

		initialised = true;
	}

	return &g_compiledScriptInterface;
}
#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif
