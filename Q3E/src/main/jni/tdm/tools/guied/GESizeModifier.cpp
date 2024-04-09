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
#include "GESizeModifier.h"

rvGESizeModifier::rvGESizeModifier ( const char* name, idWindow* window, float l, float t, float r, float b ) :
	rvGEModifier ( name, window  )
{
	mOldRect = mWrapper->GetClientRect ( );

	mNewRect[0] = mOldRect[0] + l;
	mNewRect[1] = mOldRect[1] + t;
	mNewRect[2] = mOldRect[2] + r - l;
	mNewRect[3] = mOldRect[3] + b - t;
}

bool rvGESizeModifier::Merge ( rvGEModifier* mergebase )
{
	rvGESizeModifier* merge = (rvGESizeModifier*) mergebase;
	
	mNewRect = merge->mNewRect;
			
	return true;
} 

bool rvGESizeModifier::Apply ( void )
{
	mWrapper->SetRect ( mNewRect );

	return true;
}

bool rvGESizeModifier::Undo ( void )
{
	mWrapper->SetRect ( mOldRect );
	
	return true;
}

bool rvGESizeModifier::IsValid ( void )
{
	if ( !mWindow->GetParent ( ) )
	{
		return false;
	}
	
	return true;
}
