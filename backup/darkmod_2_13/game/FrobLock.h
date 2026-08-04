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

#ifndef _FROB_LOCK_H_
#define _FROB_LOCK_H_

#include "PickableLock.h"
#include "FrobLockHandle.h"

/** 
 * greebo: This class represents a pickable lock. It supports
 * attachment of BinaryFrobMovers which are used as levers.
 */
class CFrobLock :
	public idStaticEntity
{
private:
	// The actual lock implementation
	PickableLock*	m_Lock;

	/**
	 * Handles that are associated with this lock.
	 */
	idList< idEntityPtr<CFrobLockHandle> >	m_Lockhandles;

	// The last time we issues an "Update handle" call
	int				m_LastHandleUpdateTime;

public:
	CLASS_PROTOTYPE( CFrobLock );

	CFrobLock();
	virtual ~CFrobLock();

	void			Spawn();

	bool			IsLocked();
	bool			IsPickable();

	void			Lock();
	void			Unlock();
	void			ToggleLock();

	// Attempt to open the lock, this usually triggers the handle movement
	void			Open();

	// This tries to open/lock/unlock any targetted frobmovers
	void			OpenTargets();
	void			CloseTargets();
	void			ToggleOpenTargets();
	void			LockTargets();
	void			UnlockTargets();
	void			CloseAndLockTargets();

	virtual bool	CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) override;
	virtual bool	UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item) override;

	virtual void	AttackAction(idPlayer* player) override; // Override idEntity::AttackAction to catch attack key presses from the player during lockpicking

	// Override idEntity to register the PickableLock class as object
	virtual void	AddObjectsToSaveGame(idSaveGame* savefile) override;

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

protected:
	void			PostSpawn();

	void			UpdateHandlePosition();

	// Adds a lockhandle to this lock. A lock can have multiple handles
	void			AddLockHandle(CFrobLockHandle* handle);

	/** 
	 * greebo: This automatically searches for handles bound to this lock and
	 * sets up the frob_peer, lock_handle relationship for mapper's convenience.
	 */
	void			AutoSetupLockHandles();

	virtual int		FrobLockStartSound(const char* soundName);

	virtual void	OnLock();
	virtual void	OnUnlock();

	// Required events which are called by the PickableLock class
	void			Event_Lock_StatusUpdate();
	void			Event_Lock_OnLockPicked();
	void			Event_Lock_OnLockStatusChange(int locked);

	// Script event to return the locked status
	void			Event_IsLocked();
	void			Event_IsPickable();

	// Script events for locking and unlocking
	void			Event_Lock();
	void			Event_Unlock();
	void			Event_ToggleLock();

	// Private events, used for deferred triggering of lock/unlock/general targets
	void			Event_TriggerTargets();
	void			Event_TriggerLockTargets();
	void			Event_TriggerUnlockTargets();

	// Called by the frob action script
	void			Event_Open();

	void			Event_ClearPlayerImmobilization(idEntity* player);
};

#endif /* _FROB_LOCK_H_ */
