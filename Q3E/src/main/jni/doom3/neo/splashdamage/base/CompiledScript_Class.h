// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __COMPILEDSCRIPT_CLASS_H__
#define __COMPILEDSCRIPT_CLASS_H__

#include "CompiledScript_Types.h"
#include "CompiledScriptInterface.h"

class sdCompiledScript_Event;
struct sdClassInfo;

#include "Generated_VirtualFunctionsDependencies.h"

class sdCompiledScript_Class : public sdCompiledScript_ClassBase {
public:
	SD_CS_DECLARE_CLASS( sdCompiledScript_Class );

										sdCompiledScript_Class( void ) { ; }

	static void							AddClassInfo( sdClassInfo* info );
	static void							InitClasses( void );

	static void							AddFunctionInfo( sdFunctionInfo* info );
	static void							InitFunctions( void );

	static sdClassInfo*					GetClassInfo( void ) { return classInfoChain; }
	static sdFunctionInfo*				GetFunctionInfo( void ) { return functionInfoChain; }

#include "Generated_VirtualFunctions.h"

private:
	static sdClassInfo*					classInfoChain;
	static sdFunctionInfo*				functionInfoChain;
};

#endif // __COMPILEDSCRIPT_CLASS_H__
