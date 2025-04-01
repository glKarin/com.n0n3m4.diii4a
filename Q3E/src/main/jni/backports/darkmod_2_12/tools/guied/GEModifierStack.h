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
#ifndef GEMODIFIERSTACK_H_
#define GEMODIFIERSTACK_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEModifierStack
{
public:

	rvGEModifierStack ( );
	~rvGEModifierStack ( );

	void			BlockNextMerge	( void );

	bool			Append			( rvGEModifier* modifier );
	bool			Undo			( void );
	bool			Redo			( void );
	
	void			Reset			( void );
	
	bool			CanUndo			( void );
	bool			CanRedo			( void );
	
	rvGEModifier*	GetUndoModifier	( void );
	rvGEModifier*	GetRedoModifier	( void );
		
protected:

	idList<rvGEModifier*>	mModifiers;
	int						mCurrentModifier;
	bool					mMergeBlock;
};

ID_INLINE bool rvGEModifierStack::CanRedo ( void )
{
	return mCurrentModifier < mModifiers.Num()-1;
}

ID_INLINE bool rvGEModifierStack::CanUndo ( void )
{
	return mCurrentModifier >= 0;
}

ID_INLINE void rvGEModifierStack::BlockNextMerge ( void )
{
	mMergeBlock = true;
}

ID_INLINE rvGEModifier* rvGEModifierStack::GetUndoModifier ( void )
{
	assert ( CanUndo ( ) );
	return mModifiers[mCurrentModifier];
}

ID_INLINE rvGEModifier* rvGEModifierStack::GetRedoModifier ( void )
{
	assert ( CanRedo ( ) );
	return mModifiers[mCurrentModifier+1];
}

#endif
