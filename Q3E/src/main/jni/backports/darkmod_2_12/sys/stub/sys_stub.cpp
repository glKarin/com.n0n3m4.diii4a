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
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>

#define	MAX_OSPATH			256

static	int		frameNum;

int Sys_Milliseconds( void ) {
	return frameNum * 16;
}

double Sys_GetClockTicks( void ) {
	return frameNum * 16.0;
}

double Sys_ClockTicksPerSecond( void ) {
	return 1000.0;
}

void	Sys_Sleep( int msec ) {
}

void	Sys_CreateThread(  xthread_t function, void *parms, xthreadPriority priority, xthreadInfo& info ) {
}

void Sys_DestroyThread( xthreadInfo& info ) {
}

void	Sys_FlushCacheMemory( void *base, int bytes ) {
}

void Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];

	va_start (argptr, error);
	vprintf (error, argptr);
	va_end (argptr);
	printf( "\n" );

	exit( 1 );
}

void Sys_Quit( void ) {
	exit( 0 );
}

char *Sys_GetClipboardData( void ) {
	return NULL;
}

void Sys_GenerateEvents( void ) {
}

void Sys_Init( void ) {
}

//==========================================================

idPort	clientPort, serverPort;

void Sys_InitNetworking( void ) {
}

bool idPort::GetPacket( netadr_t &net_from, void *data int &size, int maxSize ) {
	return false;
}
void idPort::SendPacket( const netadr_t to, const void *data, int size ) {
}

//==========================================================

double	idTimer::base;

void idTimer::InitBaseClockTicks( void ) const {
}

//==========================================================

void _glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
}


void Sys_InitInput( void ) {
}

void Sys_ShutdownInput( void ) {
}

sysEvent_t	Sys_GetEvent( void ) {
	sysEvent_t	ev;

	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_NONE;
	ev.evTime = Sys_Milliseconds();
	return ev;
}

void	Sys_Mkdir( const char *path ) {
}

const char *Sys_DefaultCDPath(void) {
	return "";
}

const char *Sys_DefaultBasePath(void) {
	return "";
}

int Sys_ListFiles( const char *directory, const char *extension, idStrList &list )
{
	struct dirent *d;
	DIR		*fdir;
	bool dironly = false;
	char		search[MAX_OSPATH];
	int			i;
	struct stat st;

	list.Clear();

	if ( !extension)
		extension = "";

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = true;
	}

	// search
	if ((fdir = opendir(directory)) == NULL) {
		return 0;
	}

	while ((d = readdir(fdir)) != NULL) {
		idStr::snprintf( search, sizeof(search), "%s/%s", directory, d->d_name );
		if (stat(search, &st) == -1)
			continue;
		if (!dironly) {
		    idStr look(search);
		    idStr ext;
		    look.ExtractFileExtension( ext );
		    if ( extension && extension[0] && ext.Icmp( &extension[1] ) != 0 ) {
			continue;
		    }
		}
		if ((dironly && !(st.st_mode & S_IFDIR)) ||
			(!dironly && (st.st_mode & S_IFDIR)))
			continue;

		list.Append( d->d_name );
	}

	closedir(fdir);

	return list.Num();
}

void	Sys_GrabMouseCursor( bool grabIt ) {
}

bool	Sys_StringToNetAdr( const char *s, netadr_t *a ) {
	return false;
}

const char *Sys_NetAdrToString( const netadr_t a ) {
	static char s[64];

	if ( a.type == NA_LOOPBACK ) {
		idStr::snPrintf( s, sizeof(s), "localhost" );
	} else if ( a.type == NA_IP ) {
		idStr::snPrintf( s, sizeof(s), "%i.%i.%i.%i:%i",
			a.ip[0], a.ip[1], a.ip[2], a.ip[3], BigShort(a.port) );
	}
	return s;
}

void Sys_DoPreferences( void ) {
}

int main( int argc, char **argv ) {
	// combine the args into a windows-style command line
	char	cmdline[4096];
	cmdline[0] = 0;
	for ( int i = 1 ; i < argc ; i++ ) {
		strcat( cmdline, argv[i] );
		if ( i < argc - 1 ) {
			strcat( cmdline, " " );
		}
	}
	common->Init( cmdline );

	while( 1 ) {
		common->Async();
		common->Frame();
		frameNum++;
	}
}
