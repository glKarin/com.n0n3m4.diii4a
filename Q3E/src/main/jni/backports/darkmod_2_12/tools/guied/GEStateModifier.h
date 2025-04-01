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
#ifndef GESTATEMODIFIER_H_
#define GESTATEMODIFIER_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEStateModifier : public rvGEModifier
{
public:

	rvGEStateModifier ( const char* name, idWindow* window, idDict& dict );
	
	virtual bool		Apply	( void );
	virtual bool		Undo	( void );
			
protected:

	bool	SetState	( idDict& dict );

	rvGEWindowWrapper::EWindowType	mWindowType;
	rvGEWindowWrapper::EWindowType	mUndoWindowType;
	idDict							mDict;
	idDict							mUndoDict;
}; 

#endif // GESTATEMODIFIER_H_