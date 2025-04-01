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
#include "GEInsertModifier.h"

rvGEInsertModifier::rvGEInsertModifier ( const char* name, idWindow* window, idWindow* parent, idWindow* before ) :
	rvGEModifier ( name, window )
{
	mParent  = parent;
	mBefore  = before;

	assert ( mParent );

	mUndoParent = window->GetParent ( );
	mUndoBefore = NULL;	
	mUndoRect   = mWrapper->GetClientRect ( );
	mRect		= mWrapper->GetClientRect ( );
		
	// Find the child window the window being inserted is before
	if ( mUndoParent )
	{
		int				   index;
		rvGEWindowWrapper* pwrapper;
		
		pwrapper = rvGEWindowWrapper::GetWrapper ( mUndoParent );
		
		index = mUndoParent->GetChildIndex ( mWindow );
		
		if ( index + 1 < pwrapper->GetChildCount ( ) )
		{
			mUndoBefore = pwrapper->GetChild ( index + 1 );
		}
	}		

	// Since rectangles are relative to the parent rectangle we need to figure
	// out the new x and y coordinate as if this window were a child 
	rvGEWindowWrapper* parentWrapper;		
	parentWrapper = rvGEWindowWrapper::GetWrapper ( mParent );
	mRect.x = mWrapper->GetScreenRect( )[0] - parentWrapper->GetScreenRect()[0];
	mRect.y = mWrapper->GetScreenRect( )[1] - parentWrapper->GetScreenRect()[1];
}

/*
================
rvGEInsertModifier::Apply

Apply the insert modifier by removing the child from its original parent and
inserting it as a child of the new parent
================
*/
bool rvGEInsertModifier::Apply ( void )
{
	if ( mUndoParent )
	{
		mUndoParent->RemoveChild ( mWindow );
	}
	
	mParent->InsertChild ( mWindow, mBefore );
	mWrapper->SetRect ( mRect );
	
	return true;
}

/*
================
rvGEInsertModifier::Undo

Undo the insert modifier by removing the window from the parent it was
added to and re-inserting it back into its original parent
================
*/
bool rvGEInsertModifier::Undo ( void )
{
	mParent->RemoveChild ( mWindow );
	
	if ( mUndoParent )
	{
		mUndoParent->InsertChild ( mWindow, mUndoBefore );
		mWrapper->SetRect ( mUndoRect );
	}
	
	return true;
}

