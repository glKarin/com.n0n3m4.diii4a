/*
android_lib.c - dynamic library code for Android OS
Copyright (C) 2018 Flying With Gauss

This program is free software: you can redistribute it and/sor modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#include <dlfcn.h>
#include "common.h"
#include "library.h"
#include "filesystem.h"
#include "server.h"
#include "platform/android/lib_android.h"
#include "platform/android/dlsym-weak.h" // Android < 5.0

void *ANDROID_LoadLibrary( const char *path )
{
#ifdef _DIII4A //karin: normalize library file name
	const char *libdir[2], *_name = COM_FileWithoutPath( path );
	char name[MAX_SYSPATH] = { 0 };
	if(Q_strstr(_name, "lib") != _name)
		strcat(name, "lib");
	strcat(name, _name);
	if(Q_strstr(_name, ".so") != (_name + (strlen(_name) - 3)))
		strcat(name, ".so");
#else
	const char *libdir[2], *name = COM_FileWithoutPath( path );
#endif
	char fullpath[MAX_SYSPATH];
	void *handle;

	libdir[0] = getenv( "XASH3D_GAMELIBDIR" ); // TODO: remove this once distributing games from APKs will be deprecated
	libdir[1] = NULL; // TODO: put here data directory where libraries will be downloaded to

	for( int i = 0; i < ARRAYSIZE( libdir ); i++ )
	{
		// this is an APK directory, get base path
		const char *p = i == 0 ? name : path;

		if( !libdir[i] )
			continue;
#ifdef _DIII4A //karin: print load dll path
        if( !libdir[i][0] )
            continue;
#endif

		Q_snprintf( fullpath, sizeof( fullpath ), "%s/%s", libdir[i], p );

		handle = dlopen( fullpath, RTLD_NOW );
#ifdef _DIII4A //karin: print load dll path
		printf("Load(env) %s -> %p\n", fullpath, handle);
#endif

		if( handle )
		{
			Con_Reportf( "%s: loading library %s successful\n", __func__, fullpath );
			return handle;
		}

		COM_PushLibraryError( dlerror() );
	}

#ifdef _DIII4A //karin: load library from std path
	extern const char * Sys_DLLInternalPath();
	extern const char * Sys_DLLDefaultPath();
	libdir[0] = Sys_DLLInternalPath();
	libdir[1] = Sys_DLLDefaultPath();
    if(Q_strcmp(libdir[0], libdir[1]) == 0)
        libdir[1] = NULL;

	for( int i = 0; i < ARRAYSIZE( libdir ); i++ )
	{
		// this is an APK directory, get base path
		const char *p = name; // i == 0 ? name : path;

		if( !libdir[i] || !libdir[i][0] )
			continue;

		Q_snprintf( fullpath, sizeof( fullpath ), "%s/%s", libdir[i], p );

		handle = dlopen( fullpath, RTLD_NOW );
		printf("Load(std) %s -> %p\n", fullpath, handle);

		if( handle )
		{
			Con_Reportf( "%s: loading library %s successful\n", __func__, fullpath );
			return handle;
		}

		COM_PushLibraryError( dlerror() );
	}
#endif

	// find in system search path, that includes our APK
	handle = dlopen( name, RTLD_NOW );
#ifdef _DIII4A //karin: print load dll path
	printf("Load(raw) %s -> %p\n", name, handle);
#endif
	if( handle )
	{
		Con_Reportf( "%s: loading library %s from LD_LIBRARY_PATH successful\n", __func__, name );
		return handle;
	}
	COM_PushLibraryError( dlerror() );

	return NULL;
}

void *ANDROID_GetProcAddress( void *hInstance, const char *name )
{
	void *p = dlsym( hInstance, name );

#ifndef XASH_64BIT
	if( p ) return p;

	p = dlsym_weak( hInstance, name );
#endif // XASH_64BIT

	return p;
}
