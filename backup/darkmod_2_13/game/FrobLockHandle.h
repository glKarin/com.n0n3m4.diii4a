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

#ifndef _FROB_LOCK_HANDLE_H
#define _FROB_LOCK_HANDLE_H

#include "FrobHandle.h"

class CFrobLock;

/**
 * CFrobLockHandle is meant to be used as moveable part of a CFrobLock.
 * It behaves similarly to the CFrobDoorHandle class, but is specialised for 
 * use with the static froblock.
 */
class CFrobLockHandle : 
	public CFrobHandle
{
public:
	CLASS_PROTOTYPE( CFrobLockHandle );

							CFrobLockHandle();

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	 * greebo: This method is invoked directly or it gets called by the attached master.
	 */
	virtual void			Tap() override;

	/**
	 * Get/set the lock associated with this handle.
	 */
	CFrobLock*				GetFrobLock();
	void					SetFrobLock(CFrobLock* lock);

	// greebo: Returns TRUE if the associated lock is locked (not to confuse with CBinaryFrobMover::IsLocked())
	bool					LockIsLocked();

protected:
	// Specialise the OpenPositionReached method of BinaryFrobMover to trigger the lock's open event
	virtual void			OnOpenPositionReached() override;

	// Script event interface
	void					Event_GetLock();

protected:
	/**
	* Pointer to the lock that is associated with this handle
	**/
	CFrobLock*				m_FrobLock;
};

#endif /* _FROB_LOCK_HANDLE_H */
