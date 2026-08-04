/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __SYS_POSIX__
#define __SYS_POSIX__

#include <signal.h>

void		Posix_QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
const char*	Posix_Cwd( void );

// called first thing. does InitSigs and various things
void		Posix_EarlyInit( );
// called after common has been initialized
void		Posix_LateInit( );

void		Posix_InitPThreads( );
void		Posix_InitSigs( );
void		Posix_ClearSigs( );

void		Posix_Exit( int ret );
void		Posix_SetExit(int ret); // override the exit code
void		Posix_SetExitSpawn( const char *exeName ); // set the process to be spawned when we quit

void		Posix_StartAsyncThread( void );
extern 		uintptr_t asyncThread;
extern		volatile bool asyncThreadShutdown;

bool		Posix_AddKeyboardPollEvent( int key, bool state );
bool		Posix_CanAddKeyboardPollEvent();
bool		Posix_AddMousePollEvent( int action, int value );
bool 		Posix_CanAddMousePollEvent();

void		Posix_PollInput( void );
void		Posix_InitConsoleInput( void );
void		Posix_Shutdown( void );

void		Sys_FPE_handler( int signum, siginfo_t *info, void *context );
void		Sys_DoStartProcess( const char *exeName, bool dofork = true ); // if not forking, current process gets replaced

THREAD_RETURN_TYPE		Sys_AsyncThread(void*);

double 		MeasureClockTicks();

#endif

