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

#ifndef FROBBUTTON_H
#define FROBBUTTON_H

extern const idEventDef EV_TDM_Operate;

/** 
 * angua: This class is designed specifically for buttons.
 * It doesn't do much more than using the BinaryFrobMover functions for now
 * but I guess we will want some additional functionality in the future.
 */
class CFrobButton : 
	public CBinaryFrobMover 
{
public:

	CLASS_PROTOTYPE( CFrobButton );

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	// this does only call open for now, might be that we want 
	// a more complex operation algorithm when the button is pressed in the future
	virtual void			Operate();

	// Override the base class's ApplyImpulse method
	virtual void			ApplyImpulse(idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse) override;
};

#endif /* FROBBUTTON_H */
