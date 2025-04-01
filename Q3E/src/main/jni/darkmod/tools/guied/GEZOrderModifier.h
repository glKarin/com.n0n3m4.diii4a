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
#ifndef GEZORDERMODIFIER_H_
#define GEZORDERMODIFIER_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEZOrderModifier : public rvGEModifier
{
public:

	enum EZOrderChange
	{
		ZO_FORWARD,
		ZO_BACKWARD,
		ZO_FRONT,
		ZO_BACK,
	};

	rvGEZOrderModifier ( const char* name, idWindow* window, EZOrderChange change );
	
	virtual bool		Apply	( void );
	virtual bool		Undo	( void );
	virtual bool		IsValid	( void );
			
protected:
	
	idWindow*	mBefore;
	idWindow*	mUndoBefore;
}; 

#endif // GEZORDERMODIFIER_H_