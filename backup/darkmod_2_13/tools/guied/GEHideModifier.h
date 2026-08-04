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
#ifndef GEHIDEMODIFIER_H_
#define GEHIDEMODIFIER_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEHideModifier : public rvGEModifier
{
public:

	rvGEHideModifier ( const char* name, idWindow* window, bool hide );
	
	virtual bool		Apply	( void );
	virtual bool		Undo	( void );

protected:

	bool		mHide;
	bool		mUndoHide;
	idWindow*	mParent;
};

#endif // GEHIDEMODIFIER_H_