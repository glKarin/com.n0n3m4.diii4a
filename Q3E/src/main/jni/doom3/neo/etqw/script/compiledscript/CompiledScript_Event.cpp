// Copyright (C) 2007 Id Software, Inc.
//


#include "Precompiled.h"
#pragma hdrstop

#include "CompiledScript_Event.h"
#include "CompiledScriptInterface.h"

sdCompiledScript_Event* sdCompiledScript_Event::events = NULL;

sdCompiledScript_Event::sdCompiledScript_Event( const char* _name ) {
	name	= _name;
	evt		= NULL;

	next	= events;
	events	= this;
}

void sdCompiledScript_Event::Init( void ) {
	evt = compilerInterface->FindEvent( name );
	if ( evt == NULL ) {
		assert( false );
//		throw new idCompileError( va( "Unknown Event '%s'", name ) );
	}
}

void sdCompiledScript_Event::Startup( void ) {
	for ( sdCompiledScript_Event* evt = events; evt != NULL; evt = evt->next ) {
		evt->Init();
	}
}
