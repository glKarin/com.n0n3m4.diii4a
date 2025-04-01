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



#include "RegistryOptions.h"

/*
================
rvRegistryOptions::rvRegistryOptions

Constructor
================
*/
rvRegistryOptions::rvRegistryOptions( void ) {
}

/*
================
rvRegistryOptions::Init
================
*/
void rvRegistryOptions::Init( const char *key ) {
	mBaseKey = key;
}

/*
================
rvRegistryOptions::Save

Write the options to the registry 
================
*/
bool rvRegistryOptions::Save ( void )
{
	HKEY	hKey;
	int		i;

	// Create the top level key
	if ( ERROR_SUCCESS != RegCreateKeyEx ( HKEY_LOCAL_MACHINE, mBaseKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL ) )
	{
		return false;
	}

	// Write out the values
	for ( i = 0; i < mValues.GetNumKeyVals(); i ++ )
	{
		const idKeyValue* key = mValues.GetKeyVal ( i );
		assert ( key );	
		RegSetValueEx ( hKey, key->GetKey().c_str(), 0, REG_SZ, (BYTE*)key->GetValue().c_str(), key->GetValue().Length() );
	}

	// Write Recent Files
	for ( i = 0; i < mRecentFiles.Num(); i ++ )	
	{
		RegSetValueEx ( hKey, va("mru%d",i), 0, REG_SZ, (BYTE*)mRecentFiles[i].c_str(), mRecentFiles[i].Length() );	
	}

	return true;
}

/*
================
rvRegistryOptions::Load

Read the options from the registry
================
*/
bool rvRegistryOptions::Load ( void )
{
	HKEY	hKey;
	char	temp[MAX_PATH];
	TCHAR	keyname[MAX_PATH];
	DWORD	dwType;
	DWORD	dwSize;
	int		i;

	mValues.Clear ( );
	mRecentFiles.Clear ( );

	if ( ERROR_SUCCESS != RegOpenKeyEx ( HKEY_LOCAL_MACHINE, mBaseKey, 0, KEY_READ, &hKey ) )
	{
		return false;
	}

	// Read in the values and recent files
	keyname[0] = 0;
	dwSize = MAX_PATH;
	for ( i = 0; RegEnumValue ( hKey, i, keyname, &dwSize, NULL, NULL, NULL, NULL ) == ERROR_SUCCESS; i ++ )
	{
		temp[0] = '\0';	
		dwSize = MAX_PATH;
		
		if ( ERROR_SUCCESS != RegQueryValueEx ( hKey, keyname, NULL, &dwType, (LPBYTE)temp, &dwSize ) )
		{
			continue;
		}

		dwSize = MAX_PATH;

		// Skip the mru values
		if( !idStr(keyname).IcmpPrefix ( "mru" ) )
		{
			continue;
		}
		
		mValues.Set ( keyname, temp );
	}			

	// Read Recent Files
	for ( i = 0; i < MAX_MRU_SIZE; i ++ )
	{
		dwSize = MAX_PATH;
		if ( ERROR_SUCCESS != RegQueryValueEx ( hKey, va("mru%d", i ), NULL, &dwType, (LPBYTE)temp, &dwSize ) )
		{	
			continue;
		}
		
		AddRecentFile ( temp );
	}

	return true;
}

/*
================
rvRegistryOptions::SetWindowPlacement

Set a window placement in the options
================
*/
void rvRegistryOptions::SetWindowPlacement ( const char* name, HWND hwnd )
{
	WINDOWPLACEMENT wp;
	
	wp.length = sizeof(wp);
	::GetWindowPlacement ( hwnd, &wp );
	
	idStr out;
	
	out = va("%d %d %d %d %d %d %d %d %d %d",
			 wp.flags,
			 wp.ptMaxPosition.x,
			 wp.ptMaxPosition.y,
			 wp.ptMinPosition.x,
			 wp.ptMinPosition.y,
			 wp.rcNormalPosition.left,
			 wp.rcNormalPosition.top,
			 wp.rcNormalPosition.right,
			 wp.rcNormalPosition.bottom,
			 wp.showCmd );
			 
	mValues.Set ( name, out );	 
}

/*
================
rvRegistryOptions::GetWindowPlacement

Retrieve a window placement from the options
================
*/
bool rvRegistryOptions::GetWindowPlacement ( const char* name, HWND hwnd )
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	const idKeyValue* key = mValues.FindKey ( name );
	if ( !key )
	{
		return false;
	}

	sscanf ( key->GetValue().c_str(), "%d %d %d %d %d %d %d %d %d %d",
			 &wp.flags,
			 &wp.ptMaxPosition.x,
			 &wp.ptMaxPosition.y,
			 &wp.ptMinPosition.x,
			 &wp.ptMinPosition.y,
			 &wp.rcNormalPosition.left,
			 &wp.rcNormalPosition.top,
			 &wp.rcNormalPosition.right,
			 &wp.rcNormalPosition.bottom,
			 &wp.showCmd );
			 
	::SetWindowPlacement ( hwnd, &wp );
	
	return true;
}

/*
================
rvRegistryOptions::AddRecentFile

Adds the given filename to the MRU list
================
*/
void rvRegistryOptions::AddRecentFile ( const char* filename )
{
	int i;
	
	idStr path = filename;

	// Remove duplicates first
	for ( i = mRecentFiles.Num() - 1; i >= 0; i -- )
	{
		if ( !mRecentFiles[i].Icmp ( filename ) )
		{
			mRecentFiles.RemoveIndex ( i );
			break;
		}
	}
	
	// Alwasy trip to the max MRU size
	while ( mRecentFiles.Num ( ) >= MAX_MRU_SIZE )
	{
		mRecentFiles.RemoveIndex ( 0 );
	}
		
	mRecentFiles.Append ( path );	
}

/*
================
rvRegistryOptions::SetColumnWidths

Set a group of column widths in the options
================
*/
void rvRegistryOptions::SetColumnWidths ( const char* name, HWND list )
{
	LVCOLUMN col;
	int		 index;
	idStr	 widths;
	
	col.mask = LVCF_WIDTH;	
	
	for ( index = 0; ListView_GetColumn ( list, index, &col ); index ++ )
	{
		widths += va("%d ", col.cx );
	}
	
	mValues.Set ( name, widths );
}

/*
================
rvRegistryOptions::GetColumnWidths

Retrieve a group of column widths from the options
================
*/
void rvRegistryOptions::GetColumnWidths ( const char* name, HWND list )
{
	idStr		widths;
	const char* parse;
	const char* next;
	int			index;
	
	widths = mValues.GetString ( name );
	parse = widths;
	index = 0;
	
	while ( NULL != (next = strchr ( parse, ' ' ) ) )
	{
		int width;
		
		sscanf ( parse, "%d", &width );
		parse = next + 1;
		
		ListView_SetColumnWidth ( list, index++, width );
	}
}

/*
================
rvRegistryOptions::SetBinary

Set binary data for the given key
================
*/
void rvRegistryOptions::SetBinary ( const char* name, const unsigned char* data, int size )
{
	idStr binary;
	for ( size --; size >= 0; size --, data++ )
	{
		binary += va("%02x", *data );
	}
	
	mValues.Set ( name, binary );
}

/*
================
rvRegistryOptions::GetBinary

Get the binary data for a given key
================
*/
void rvRegistryOptions::GetBinary ( const char* name, unsigned char* data, int size )
{
	const char* parse;
	parse = mValues.GetString ( name );
	for ( size --; size >= 0 && *parse && *(parse+1); size --, parse += 2, data ++  )
	{
		int value;
		sscanf ( parse, "%02x", &value );
		*data = (unsigned char)value;
	}	
}
