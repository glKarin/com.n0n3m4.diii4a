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



#include "GEApp.h"
#include "GEDeleteModifier.h"

rvGEDeleteModifier::rvGEDeleteModifier ( const char* name, idWindow* window ) :
	rvGEModifier ( name, window )
{
}

/*
================
rvGEDeleteModifier::Apply

Apply the delete modifier by setting the deleted flag in the wrapper
================
*/
bool rvGEDeleteModifier::Apply ( void )
{
	mWrapper->SetDeleted ( true );
	
	return true;
}

/*
================
rvGEDeleteModifier::Undo

Undo the delete modifier by unsetting the deleted flag in the wrapper
================
*/
bool rvGEDeleteModifier::Undo ( void )
{
	mWrapper->SetDeleted ( false );
	
	return true;
}

