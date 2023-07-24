// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __COMPILEDSCRIPT_EVENT_H__
#define __COMPILEDSCRIPT_EVENT_H__

class idEventDef;

class sdCompiledScript_Event {
public:
							sdCompiledScript_Event( const char* _name );

	void					Init( void );
	const idEventDef*		GetEvent( void ) const { return evt; }

	static void				Startup( void );

private:
	const char*				name;
	const idEventDef*		evt;
	sdCompiledScript_Event*	next;

	static sdCompiledScript_Event* events;
};

#endif // __COMPILEDSCRIPT_EVENT_H__
