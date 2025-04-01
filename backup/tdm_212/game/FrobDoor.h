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

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#ifndef FROBDOOR_H
#define FROBDOOR_H

class CFrobDoorHandle;

// Number of clicksounds available
#define	MAX_PIN_CLICKS			14

// Number of clicks that have to be set as a minimum. A pattern of less than 
// 5 clicks is very easy to recognize, so it doesn't make sense to allow less than that.
#define MIN_CLICK_NUM			5
#define MAX_CLICK_NUM			10

// grayman #3643 - indices into door-handling position array
#define NUM_DOOR_POSITIONS 4 // Front, Back, Mid door-handling positions, and side marker
#define DOOR_POS_FRONT  0
#define DOOR_POS_BACK   1
#define DOOR_POS_MID    2
#define DOOR_POS_SIDEMARKER 3

#define DOOR_SIDES      2 // Front side, back side
#define DOOR_SIDE_FRONT 0
#define DOOR_SIDE_BACK  1
#define AI_SIZE			32

/**
 * CFrobDoor is a replacement for idDoor. The reason for this replacement is
 * because idDoor is derived from idMover_binary and can only slide from one
 * position into another. In order to make swinging doors we need to rotate
 * them but this doesn't work with normal idDoors. So CFrobDoor is a mixture
 * of idDoor and idMover.
 */
class CFrobDoor : 
	public CBinaryFrobMover
{
public:
	CLASS_PROTOTYPE( CFrobDoor );

							CFrobDoor();

	virtual					~CFrobDoor() override;

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Open(bool Master) override;
	virtual void			Close(bool Master) override;

	virtual void			Think( void ) override; // grayman #3042 - need to think while moving

	/** 
	 * greebo: The OpenDoor method is necessary to give the FrobDoorHandles a 
	 * "low level" open routine. The CFrobDoor::Open() call is re-routed to 
	 * the FrobDoorHandle::Tap() method, so there must be a way to actually
	 * let the door open. Which is what this method does.
	 */
	virtual void			OpenDoor(bool Master);		

	virtual void			Lock(bool Master) override;
	virtual bool			Unlock(bool Master) override; // grayman #3643

	CFrobDoorHandle*		GetDoorhandle();
	// Adds a door handle to this door. A door can have multiple handles
	void					AddDoorhandle(CFrobDoorHandle* handle);

	virtual bool			CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) override;
	virtual bool			UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item) override;

	// Override idEntity::AttackAction to catch attack key presses from the player during lockpicking
	virtual void			AttackAction(idPlayer* player) override;

	/**
	 * Write the proper sound loss value to the soundprop portal data
	 * Called when door spawns, is and when it is opened or closed
	 **/
	void					UpdateSoundLoss();

	/**
	 * Return the double door.  Returns NULL if there is none.
	 **/
	CFrobDoor*				GetDoubleDoor();

	/**
	 * Return the number of controllers targeting the door.
	 **/
	int						GetControllerNumber(); // grayman 5109

	/**
	 * Close the visportal, but only if the double door is also closed.
	 **/
	virtual void			ClosePortal() override;

	// Override the idEntity frob methods
	virtual void			SetFrobbed(const bool val) override;
	virtual bool			IsFrobbed() const override;

	// angua: returns the number of open peers
	ID_INLINE int			GetOpenPeersNum()
	{
		return m_OpenPeers.Num();
	}

	/**
	 * greebo: Override the BinaryFrobMover function to re-route all sounds
	 * to the doorhandle. This avoids sounds being played from door origins,
	 * which is barely audible to the player.
	 */
	virtual int				FrobMoverStartSound(const char* soundName) override;

	/** 
	 * greebo: Override the standard idEntity method so sounds are emitted from the nearest position 
	 * to the player instead of from the bounding box center, which might be on the far side
	 * of a closed portal. This method gets applied to doors without handles, usually.
	 */
	virtual bool			GetPhysicsToSoundTransform(idVec3 &origin, idMat3 &axis) override;

	void					SetLastUsedBy(idEntity* ent);	// grayman #2859
	idEntity*				GetLastUsedBy() const;			// grayman #2859
	void					SetSearching(idEntity* ent);	// grayman #2866
	idEntity*				GetSearching() const;			// grayman #2866
	void					SetWasFoundLocked(bool state);	// grayman #3104
	bool					GetWasFoundLocked() const;		// grayman #3104
	bool					GetDoorHandlingEntities(idAI* owner, idList< idEntityPtr<idEntity> > &list); // grayman #2866
	void					SetLossBase( float lossAI, float lossPlayer ); // grayman #3042
//	bool					GetDoorControllers( idAI* owner, idList< idEntityPtr<idEntity> > &list ); // grayman #3643

	// grayman #3643 - add a controller to the controller list
	void					AddController(idEntity* newController);

	// grayman #3643 - add a door_handling_position to the door handle position list
	void					AddDHPosition(idEntity* newDHPosition);

	// grayman #3643 - register door opening data for each side of the door
	void					GetDoorHandlingPositions();

	// grayman #3643 - xfer the door's locking situation to the controllers
	void					SetControllerLocks();

	// grayman #3643 - find the mid position for door handling
	void					GetMidPos(float rotationAngle);

	// grayman #3755 - find the side markers for door handling
	void					GetSideMarkers();

	// grayman #3643 - find positions for rotating door handling
	void					GetForwardPos();
	void					GetBehindPos();

	// grayman #3643 - retrieve a particular door position
	idVec3					GetDoorPosition(int side, int position)
							{ return m_doorPositions[side][position];}

	// grayman #3643 - retrieve a particular door controller
	idEntityPtr<idEntity>	GetDoorController(int side);

	// grayman #4882 - retrieve a peek entity
	idEntityPtr<idPeek>		GetDoorPeekEntity();
	
	// grayman #3643 - retrieve a particular door handle position
	idEntityPtr<idEntity>	GetDoorHandlePosition(int side);

	// grayman #3643 - lock and unlock all door controllers
	void					LockControllers(bool bMaster);
	bool					UnlockControllers(bool bMaster);

	// grayman #3643 - IsLocked() now has to deal with testing door controllers if used
	virtual bool			IsLocked() override;

	void					PushDoorHard();		 // grayman #3748
	void					StopPushingDoorHard(); // grayman #3748
	bool					IsPushingHard();	 // grayman #3755

protected:

	// Returns the handle nearest to the given position
	CFrobDoorHandle*		GetNearestHandle(const idVec3& pos) const;

	/**
	 * This will read the spawnargs lockpick_bar, lockpick_rotate and 
	 * lockpick_translate, to setup the parameters how the bar or handle should behave
	 * while it is picked. Also other intialization stuff, that can only be done after all
	 * the entities are loaded, should be done here.
	 */
	virtual void			PostSpawn() override;

	void					Event_PostPostSpawn(); // grayman #3643

	// angua: flag the AAS area the door is located in with the travel flag TFL_DOOR
	virtual void			SetDoorTravelFlag();
	virtual void			ClearDoorTravelFlag();

	/**
	 * Find out if this door is touching another door, and if they share the same portal
	 * If so, store a pointer to the other door m_DoubleDoor on this door.
	 *
	 * This is posted as an event to be called on all doors after entities spawn
	 **/
	void					FindDoubleDoor();

	/** 
	 * greebo: This automatically searches for handles bound to this door and
	 * sets up the frob_peer, door_handle relationship for mapper's convenience.
	 */
	void					AutoSetupDoorHandles();

	/**
	 * greebo: This is the algorithm for linking the double door via open_peer and lock_peer.
	 */
	void					AutoSetupDoubleDoor();

	// angua: Specialise the CBinaryFrobMover::PreLock method to check whether lock peers are closed
	virtual bool			PreLock(bool bMaster) override;

	// Specialise the CBinaryFrobMover::OnLock() and OnUnlock() methods to update the peers
	virtual void			OnLock(bool bMaster) override;
	virtual bool			OnUnlock(bool bMaster) override; // grayman #3643

	// Specialise the OnStartOpen/OnStartClose event to send the call to the open peers
	virtual void			OnStartOpen(bool wasClosed, bool bMaster) override;
	virtual void			OnStartClose(bool wasOpen, bool bMaster) override;

	// Gets called when the mover finishes its closing move and is fully closed (virtual override)
	virtual void			OnClosedPositionReached() override;

	// Helper functions to cycle through the m_OpenList members
	void					OpenPeers();
	void					ClosePeers();
	void					OpenClosePeers(const bool open);

	// Taps all doorhandles of open peers
	void					TapPeers();

	void					LockPeers();
	void					UnlockPeers();
	void					LockUnlockPeers(const bool lock);

	// Accessor functions for adding and removing peers
	void					AddOpenPeer(const idStr& peerName);
	void					RemoveOpenPeer(const idStr& peerName);

	void					AddLockPeer(const idStr& peerName);
	void					RemoveLockPeer(const idStr& peerName);

	// Returns TRUE if all lock peer doors are at their respective "closed" position
	bool					AllLockPeersAtClosedPosition();

	// Updates the position of the attached handles according to the current lockpick state
	void					UpdateHandlePosition();

	// Required events which are called by the PickableLock class
	virtual void			Event_Lock_StatusUpdate() override;
	virtual void			Event_Lock_OnLockPicked() override;

	// Script event interface
	void					Event_GetDoorhandle();
	void					Event_OpenDoor(float master);

	// This is called periodically, to handle a pending close request (used for locking doors after closing)
	void					Event_HandleLockRequest();

protected:

	/**
	 * This is a list of slave doors, which should be opened and closed
	 * along with this door.
	 */
	idList<idStr>				m_OpenPeers;

	/** 
	 * This list is the pendant to the above one: m_OpenPeers. It specifies
	 * all names of the doors which should be locked/unlocked along with this one.
	 */
	idList<idStr>				m_LockPeers;

	/**
	* Pointer to the door's partner in a double door.
	* Double door means two doors placed right next to eachother, sharing the
	*	same visportal.
	* 
	* The doubledoor does not necessarily have to be linked in a frob chain,
	*	it could be independently opened.
	**/
	idEntityPtr<CFrobDoor>		m_DoubleDoor;

	/**
	 * Handles associated with this door.
	 */
	idList< idEntityPtr<CFrobDoorHandle> >	m_Doorhandles;

	// The last time we issues an "Update handle" call
	int							m_LastHandleUpdateTime;

	/**
	* grayman #3042 - sound loss values
	**/
	float						m_lossOpen;
	float						m_lossDoubleOpen;
	float						m_lossClosed;
	float						m_lossBaseAI;
	float						m_lossBasePlayer;

	/**
	* grayman #3042 - m_isTransparent set to 1 means don't close a visportal when closing.
	* this allows us to run a visportal through this type of door, associating it with the
	* door, but not closing it when the door is closed.
	**/
	bool						m_isTransparent;

	/**
	* grayman #3643 - holds door-handling positions, 3 for each of 2 sides
	**/
	idVec3						m_doorPositions[DOOR_SIDES][NUM_DOOR_POSITIONS];

	/**
	* grayman #3643 - holds door controllers
	**/
	idList< idEntityPtr<idEntity> > m_controllers;

	/**
	* grayman #4882 - holds a peek entity
	**/
	idEntityPtr<idPeek> m_peek;

	/**
	* grayman #3643 - holds door handle positions
	**/
	idList< idEntityPtr<idEntity> > m_doorHandlingPositions;

	/**
	* grayman #3643 - true if the door rotates, false if it slides
	**/
	bool						m_rotates;

	/**
	* grayman #3748 - previous push setting
	**/
	bool						m_previouslyPushingPlayer;

	/**
	* grayman #3748 - previous frob setting
	**/
	bool						m_previouslyFrobable;

	/**
	* grayman #3748 - AI is pushing the door
	**/
	bool						m_AIPushingDoor;

	/**
	* grayman #3755 - Variable, depending on door size, to be
	* used for the speed rate changes when an AI is in a hurry.
	* Determined only once, at spawn time.
	*
	* = 2.0 when door face area is <= 7168
	* = 1.0 when door face area is >= 10752
	* between 1.0 and 2.0 on a sliding scale between 7168 and 10752
	*
	**/
	float						m_speedFactor;
};

#endif /* FROBDOOR_H */
