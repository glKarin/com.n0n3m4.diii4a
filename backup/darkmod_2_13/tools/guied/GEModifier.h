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
#ifndef GEMODIFIER_H_
#define GEMODIFIER_H_

class idWindow;
class rvGEWindowWrapper;

class rvGEModifier
{
public:

	rvGEModifier ( const char* name, idWindow* window );
	virtual ~rvGEModifier ( ) { }

	virtual bool		Apply		( void ) = 0;
	virtual bool		Undo		( void ) = 0;	
	virtual const char*	GetName		( void );
	virtual bool		CanMerge	( rvGEModifier* merge );
	
	virtual bool		IsValid		( void );
	
	virtual bool		Merge		( rvGEModifier* merge );
	
	idWindow*			GetWindow	( void );
	
	
protected:
	
	idWindow*			mWindow;
	rvGEWindowWrapper*	mWrapper;
	idStr				mName;
};

ID_INLINE bool rvGEModifier::IsValid ( void )
{
	return true;
}

ID_INLINE idWindow* rvGEModifier::GetWindow ( void )
{
	return mWindow;
}

ID_INLINE const char* rvGEModifier::GetName ( void )
{
	return mName;
}

ID_INLINE bool rvGEModifier::CanMerge ( rvGEModifier* merge )
{
	return false;
}

ID_INLINE bool rvGEModifier::Merge ( rvGEModifier* merge )
{
	return false;
}

#endif // GEMODIFIER_H_
