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

#ifndef _FROB_HANDLE_H
#define _FROB_HANDLE_H

/**
 * CFrobHandle is a generic binary mover meant to be attached to other
 * entities like doors and froblocks. This is just a base class - there are
 * actual implementations like CFrobDoorHandle and CFrobLockHandle to be
 * used as handle for the corresponding classes CFrobDoor and CFrobLock.
 *
 * A frob handle re-routes the frob- and attack actions to its master entity.
 *
 * For instance: when frobbing a door with a handle attached, the event chain is like this:
 * Frob > Door::Open > Handle::Tap > Handle moves to open pos > Door::OpenDoor
 */
class CFrobHandle : 
	public CBinaryFrobMover
{
public:
	CLASS_PROTOTYPE( CFrobHandle );

							CFrobHandle();

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	 * Functions that must be forwarded to the master.
	 */
	virtual void			SetFrobbed(const bool val) override;
	virtual bool			IsFrobbed() const override;

	// Action function, these are re-routed to the frobmaster entity
	virtual bool			CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) override;
	virtual bool			UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item) override;
	virtual void			AttackAction(idPlayer* player) override;

	// stgatilov: this method does NOT override idEntity::FrobAction and is never called
	// this has been so for ages, so I'm not sure I want to change it now
	//void					FrobAction(bool bMaster) override;

	// These functions need to be disabled on the handle. Therefore
	// they are provided but empty.
	virtual void			ToggleLock() override;

	// Returns the associated master entity
	void					SetFrobMaster(idEntity* frobMaster);
	idEntity*				GetFrobMaster();

	/**
	 * greebo: This method is invoked directly or it gets called by the attached master.
	 * For instance, a call to CFrobDoor::Open() gets re-routed here first to let 
	 * the handle animation play before actually trying to open the door.
	 */
	virtual void			Tap();

	/** 
	 * greebo: Accessor methods for the "master" flag. If a door or lock has multiple
	 * handles, only one is allowed to open the lock/door, all others are dummies.
	 *
	 * Note: These methods are mainly for "internal" use by the master classes.
	 */
	bool					IsMasterHandle();
	void					SetMasterHandle(bool isMaster);

	/** 
	 * greebo: Override the standard idEntity method to emit sounds from the nearest position 
	 * to the player instead of the bounding box center.
	 */
	virtual bool			GetPhysicsToSoundTransform(idVec3 &origin, idMat3 &axis) override;

protected:
	// Script event interface
	void					Event_Tap();

protected:
	/**
	 * greebo: The master entity to be queried for frob-related functions. 
	 * If the master entity is frobbed, this entity is considered "frobbed" as well.
	 */
	idEntity*				m_FrobMaster;

	/**
	 * A master entity can have multiple handles attached, but only the master
	 * handle is allowed to call OpenDoor() to avoid double-triggering.
	 *
	 * This bool defaults to TRUE at spawn time, so the mapper doesn't need
	 * to care about that. If a door has multiple handles, the auto-setup
	 * algorithm takes care of "deactivating" all handles but one.
	 */
	bool					m_IsMasterHandle;

	// A mutable bool to avoid infinite loops when propagating the frob
	bool					m_FrobLock;
};

#endif /* _FROB_HANDLE_H */
