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
#include "GEStateModifier.h"

rvGEStateModifier::rvGEStateModifier ( const char* name, idWindow* window, idDict& dict ) :
	rvGEModifier ( name, window ),
	mDict ( dict )
{
	mDict.Copy ( dict );

	// Make a copy of the current dictionary
	mUndoDict.Copy ( mWrapper->GetStateDict() );
}

/*
================
rvGEStateModifier::Apply

Applys the new state dictionary to the window
================
*/
bool rvGEStateModifier::Apply ( void )
{	
	return SetState ( mDict );
}

/*
================
rvGEStateModifier::Undo

Applies the undo dictionary to the window
================
*/
bool rvGEStateModifier::Undo ( void )
{
	return SetState ( mUndoDict );
}

/*
================
rvGEStateModifier::Apply

Applys the given dictionary to the window
================
*/
bool rvGEStateModifier::SetState ( idDict& dict )
{
	const idKeyValue*	key;
	int					i;
	
	// Delete any key thats gone in the new dict
	for ( i = 0; i < mWrapper->GetStateDict().GetNumKeyVals(); i ++ )
	{
		key = mWrapper->GetStateDict().GetKeyVal ( i );
		if ( !key )
		{
			continue;
		}
	}
	
	mWrapper->SetState ( dict );
	
	return true;
}

