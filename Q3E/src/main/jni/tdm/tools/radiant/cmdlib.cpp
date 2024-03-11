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

#include "precompiled.h"
#pragma hdrstop



#include "qe3.h"
#include "cmdlib.h"

#define PATHSEPERATOR   '/'

// rad additions
// 11.29.99
PFN_ERR *g_pfnError = NULL;
PFN_PRINTF *g_pfnPrintf = NULL;
PFN_ERR_NUM *g_pfnErrorNum = NULL;
PFN_PRINTF_NUM *g_pfnPrintfNum = NULL;


void Error(const char *pFormat, ...)
{
	if (g_pfnError)
	{
		va_list arg_ptr;
		va_start(arg_ptr, pFormat);
		g_pfnError(pFormat, arg_ptr);
		va_end(arg_ptr);
	}
}

void Printf(const char *pFormat, ...)
{
	if (g_pfnPrintf)
	{
		va_list arg_ptr;
		va_start(arg_ptr, pFormat);
		g_pfnPrintf(pFormat, arg_ptr);
		va_end(arg_ptr);
	}
}

void ErrorNum(int nErr, const char *pFormat, ...)
{
	if (g_pfnErrorNum)
	{
		va_list arg_ptr;
		va_start(arg_ptr, pFormat);
		g_pfnErrorNum(nErr, pFormat, arg_ptr);
		va_end(arg_ptr);
	}
}

void PrintfNum(int nErr, const char *pFormat, ...)
{
	if (g_pfnPrintfNum)
	{
		va_list arg_ptr;
		va_start(arg_ptr, pFormat);
		g_pfnPrintfNum(nErr, pFormat, arg_ptr);
		va_end(arg_ptr);
	}
}

void SetErrorHandler(PFN_ERR pe)
{
	g_pfnError = pe;
}

void SetPrintfHandler(PFN_PRINTF pe)
{
	g_pfnPrintf = pe;
}

void SetErrorHandlerNum(PFN_ERR_NUM pe)
{
	g_pfnErrorNum = pe;
}

void SetPrintfHandler(PFN_PRINTF_NUM pe)
{
	g_pfnPrintfNum = pe;
}


/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

/*
==============
LoadFile
==============
*/
int LoadFile (const char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	*bufferptr = NULL;

	if ( filename == NULL || strlen(filename) == 0 ) {
		return -1;
	}

	f = fopen( filename, "rb" );
	if ( !f ) {
		return -1;
	}
	length = Q_filelength( f );
	buffer = Mem_ClearedAlloc( length+1 );
	((char *)buffer)[length] = 0;
	if ( (int)fread( buffer, 1, length, f ) != length ) {
		Error( "File read failure" );
	}
	fclose( f );

	*bufferptr = buffer;
	return length;
}

/*
==============
DefaultExtension
==============
*/
void DefaultExtension (char *path, char *extension)
{
	char    *src;
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}

/*
==============
DefaultPath
==============
*/
void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}

/*
==============
StripFilename
==============
*/
void StripFilename (char *path)
{
    size_t length = strlen(path) - 1;
	while (length > 0 && path[length] != PATHSEPERATOR) {
		length--;
	}
	path[length] = 0;
}

/*
==============
StripExtension
==============
*/
void StripExtension (char *path)
{
	size_t length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length) {
		path[length] = 0;
	}
}
