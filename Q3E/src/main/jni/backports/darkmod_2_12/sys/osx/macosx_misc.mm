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

#define GL_GLEXT_LEGACY // AppKit.h include pulls in gl.h already
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include "../../idlib/precompiled.h"
#include "../sys_local.h"

/*
==================
idSysLocal::OpenURL
==================
*/
void idSysLocal::OpenURL( const char *url, bool doexit ) {
	static bool	quit_spamguard = false;

	if ( quit_spamguard ) {
		common->DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf("Open URL: %s\n", url);

	
	[[ NSWorkspace sharedWorkspace] openURL: [ NSURL URLWithString: 
		[ NSString stringWithCString: url ] ] ];

	if ( doexit ) {
		quit_spamguard = true;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==================
Sys_DoStartProcess
==================
*/
void Sys_DoStartProcess( const char *exeName, bool dofork = true ) {
	common->Printf( "TODO: Sys_DoStartProcess %s\n", exeName );
}

/*
==================
OSX_GetLocalizedString
==================
*/
const char* OSX_GetLocalizedString( const char* key )
{
	NSString *string = [ [ NSBundle mainBundle ] localizedStringForKey:[ NSString stringWithCString: key ]
													 value:@"No translation" table:nil];
	return [string cString];
}
