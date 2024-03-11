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

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "GameWindow.h"

/*
================
idGameWindowProxy::idGameWindowProxy
================
*/
idGameWindowProxy::idGameWindowProxy( idDeviceContext *d, idUserInterfaceLocal *g ) : idWindow( d, g ) { }

/*
================
idGameWindowProxy::Draw
================
*/
void idGameWindowProxy::Draw( int time, float x, float y ) {
	common->Printf( "TODO: idGameWindowProxy::Draw\n" );
}

