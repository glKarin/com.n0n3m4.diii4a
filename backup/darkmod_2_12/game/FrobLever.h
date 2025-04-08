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

#ifndef _FROB_LEVER_H_
#define _FROB_LEVER_H_

/** 
 * greebo: This class is designed specifically for levers.
 *
 * It builds on top of the BinaryFrobMover class and overrides
 * the relevant virtual event functions to implement proper 
 * two-state lever behaviour.
 */
class CFrobLever : 
	public CBinaryFrobMover 
{
public:

	CLASS_PROTOTYPE( CFrobLever );

	void			Spawn();

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

	// Switches the lever state to the given state (true = "open")
	void			SwitchState(bool newState);

	// Calling Operate() toggles the current state
	void			Operate();

protected:
	// Specialise the postspawn event 
	virtual void	PostSpawn() override;

	// Specialise the BinaryFrobMover events
	virtual void	OnOpenPositionReached() override;
	virtual void	OnClosedPositionReached() override;

protected:
	// The latch is to keep track of our visited positions.
	// For instance, the lever should not trigger the targets at
	// its closed position when it wasn't fully opened before.
	bool			m_Latch;

private:
	// Script interface
	void			Event_Operate();
	void			Event_Switch(int newState);
};

#endif /* _FROB_LEVER_H_ */
