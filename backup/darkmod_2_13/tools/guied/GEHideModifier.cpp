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
#include "GEHideModifier.h"

rvGEHideModifier::rvGEHideModifier ( const char* name, idWindow* window, bool hide ) :
	rvGEModifier ( name, window )
{
	mParent		= NULL;
	mHide		= hide;
	mUndoHide	= mWrapper->IsHidden ( );

	// If unhiding then find any parent window along the way that may be hidden and prevent
	// this window from being visible
	if ( !hide )
	{
		mParent = mWindow;
		while ( NULL != (mParent = mParent->GetParent ( ) ) )
		{
			if ( rvGEWindowWrapper::GetWrapper(mParent)->IsHidden ( ) )
			{
				break;
			}
		}
	}
}

/*
================
rvGEHideModifier::Apply

Apply the hide modifier by setting the visible state of the wrapper window
================
*/
bool rvGEHideModifier::Apply ( void )
{
	mWrapper->SetHidden ( mHide );

	if ( mParent )
	{
		rvGEWindowWrapper::GetWrapper ( mParent )->SetHidden ( mHide );
	}
		
	return true;
}

/*
================
rvGEHideModifier::Undo

Undo the hide modifier by setting the undo visible state of the wrapper window
================
*/
bool rvGEHideModifier::Undo ( void )
{
	mWrapper->SetHidden ( mUndoHide );

	if ( mParent )
	{
		rvGEWindowWrapper::GetWrapper ( mParent )->SetHidden ( mUndoHide );
	}
	
	return true;
}

