// Copyright (C) 2007 Id Software, Inc.
//


#include "Precompiled.h"
#pragma hdrstop

#include "CompiledScript_Class.h"

SD_CS_IMPLEMENT_CLASS( sdCompiledScript_Class, sdCompiledScript_ClassBase );

sdClassInfo* sdCompiledScript_Class::classInfoChain = NULL;
sdFunctionInfo* sdCompiledScript_Class::functionInfoChain = NULL;

void sdCompiledScript_Class::AddClassInfo( sdClassInfo* info ) {
	info->next = classInfoChain;
	classInfoChain = info;
}

void sdCompiledScript_Class::AddFunctionInfo( sdFunctionInfo* info ) {
	info->next = functionInfoChain;
	functionInfoChain = info;
}
