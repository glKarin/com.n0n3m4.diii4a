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
#ifndef GEKEYVALUEMODIFIER_H_
#define GEKEYVALUEMODIFIER_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEKeyValueModifier : public rvGEModifier
{
public:

	rvGEKeyValueModifier ( const char* name, idWindow* window, const char* key, const char* value );
	
	virtual bool		Apply		( void );
	virtual bool		Undo		( void );

	virtual bool		CanMerge	( rvGEModifier* merge );
	virtual bool		Merge		( rvGEModifier* merge );
			
protected:
	
	idStr		mKey;
	idStr		mValue;
	idStr		mUndoValue;
}; 

ID_INLINE bool rvGEKeyValueModifier::CanMerge ( rvGEModifier* merge )
{
	return !((rvGEKeyValueModifier*)merge)->mKey.Icmp ( mKey );
}

#endif // GEKEYVALUEMODIFIER_H_