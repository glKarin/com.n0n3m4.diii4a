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



#include "../../renderer/tr_local.h"
#include "../../sys/win32/win_local.h"
#include <io.h>

#include "../../ui/DeviceContext.h"
#include "../../sys/win32/rc/guied_resource.h"

#include "GEApp.h"

rvGEApp		gApp;
	
/*
================
GUIEditorInit

Start the gui editor
================
*/
void GUIEditorInit( void ) 
{
	gApp.Initialize();
}

/*
================
GUIEditorShutdown
================
*/
void GUIEditorShutdown( void ) {
}

/*
================
GUIEditorHandleMessage

Handle translator messages
================
*/
bool GUIEditorHandleMessage ( void *msg )
{
	if ( !gApp.IsActive ( ) )
	{
		return false;
	}

	return gApp.TranslateAccelerator( reinterpret_cast<LPMSG>(msg) );
}

/*
================
GUIEditorRun

Run a frame 
================
*/
void GUIEditorRun() 
{
    MSG			msg;

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) 
	{
		if ( !GetMessage (&msg, NULL, 0, 0) ) 
		{
			common->Quit();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		if ( win32.sysMsgTime && win32.sysMsgTime > (int)msg.time ) 
		{
		} 
		else 
		{
			win32.sysMsgTime = msg.time;
		}

		if ( gApp.TranslateAccelerator ( &msg ) )
		{
			continue;
		}
 
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	gApp.RunFrame ( );
	
	// The GUI editor runs too hot so we need to slow it down a bit.
	Sleep ( 1 );
}

/*
================
StringFromVec4

Returns a clean string version of the given vec4
================
*/
const char *StringFromVec4 ( idVec4& v )
{
	return va( "%s,%s,%s,%s",
		idStr::FloatArrayToString( &v[0], 1, 8 ),
		idStr::FloatArrayToString( &v[1], 1, 8 ),
		idStr::FloatArrayToString( &v[2], 1, 8 ),
		idStr::FloatArrayToString( &v[3], 1, 8 ) );
}

/*
================
IsExpression

Returns true if the given string is an expression
================
*/
bool IsExpression ( const char* s )
{
    idParser src(s, static_cast<int>(strlen(s)), "",
				  LEXFL_ALLOWMULTICHARLITERALS		| 
				  LEXFL_NOSTRINGCONCAT				| 
				  LEXFL_ALLOWBACKSLASHSTRINGCONCAT	|
				  LEXFL_NOFATALERRORS );

	idToken token;
	bool	needComma = false;
	bool	needNumber = false;
	while ( src.ReadToken ( &token ) )
	{
		switch ( token.type )
		{
			case TT_NUMBER:
				needComma = true;
				needNumber = false;
				break;
			
			case TT_PUNCTUATION:
				if ( needNumber )
				{
					return true;
				}				
				if ( token[0] == ',' )
				{
					if ( !needComma )
					{
						return true;
					}
					
					needComma = false;
					break;
				}

				if ( needComma )
				{
					return true;
				}

				if ( token[0] == '-' )
				{
					needNumber = true;
				}
				break;
				
			default:
				return true;
		}
	}
					
	return false;
}
