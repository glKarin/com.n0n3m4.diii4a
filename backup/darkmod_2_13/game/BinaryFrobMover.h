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

#ifndef BINARYFROBMOVER_H
#define BINARYFROBMOVER_H

#include "PickableLock.h"

// Forward declare the events
extern const idEventDef EV_TDM_FrobMover_Open;
extern const idEventDef EV_TDM_FrobMover_Close;
extern const idEventDef EV_TDM_FrobMover_ToggleOpen;
extern const idEventDef EV_TDM_FrobMover_Lock;
extern const idEventDef EV_TDM_FrobMover_Unlock;
extern const idEventDef EV_TDM_FrobMover_ToggleLock;
extern const idEventDef EV_TDM_FrobMover_IsOpen;
extern const idEventDef EV_TDM_FrobMover_IsLocked;
extern const idEventDef EV_TDM_FrobMover_HandleLockRequest;

#define LOCK_REQUEST_DELAY 250 // msecs before a mover locks itself after closing (if the preference is set appropriately)

/**
 * CBinaryFrobMover is a replacement for idDoor. The reason for this replacement is
 * because idDoor is derived from idMover_binary and can only slide from one
 * position into another. In order to make swinging doors we need to rotate
 * them but this doesn't work with normal idDoors. So CBinaryFrobMover is a mixture
 * of idDoor and idMover.
 */
class CBinaryFrobMover : 
	public idMover
{
public:
	CLASS_PROTOTYPE( CBinaryFrobMover );

	// Constructor
	CBinaryFrobMover();

	virtual ~CBinaryFrobMover();

	void					Spawn();
	void					Event_PostSpawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	// Override idEntity to register the PickableLock class.
	virtual void			AddObjectsToSaveGame(idSaveGame* savefile) override;

	/**
	 * greebo: A set of convenience methods, which set the master bool to TRUE.
	 * Don't use default argument initialisers on the virtual functions, 
	 * as the default values are statically bound and lead to problems 
	 * if subclasses want to override that default value.
	 */
	ID_INLINE void			Open()		{ Open(true);	}
	ID_INLINE void			Close()		{ Close(true);	}
	ID_INLINE void			Lock()		{ Lock(true);	}
	ID_INLINE void			Unlock()	{ Unlock(true);	}
	
	virtual void			Open(bool Master);
	virtual void			Close(bool Master);
	virtual void			Lock(bool Master);
	virtual bool			Unlock(bool Master); // grayman #3643

	virtual void			ToggleOpen();
	virtual void			ToggleLock();
	virtual bool			CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) override; // grayman #3643
	virtual bool			UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item) override; // grayman #3643
	virtual void			Event_ClearPlayerImmobilization(idEntity* player); // grayman #3643
	virtual void			Event_Lock_StatusUpdate(); // grayman #3643
	virtual void			Event_Lock_OnLockPicked(); // grayman #3643
	virtual void			Event_GetFractionalPosition(void); // grayman #4433

	// Performs a "delayed" lock, closes the mover and tries to lock it afterwards
	virtual void			CloseAndLock();

	// Ishtvan: Allow for fine control when frob is held
	virtual void			FrobAction(bool frobMaster, bool isFrobPeerAction = false) override;
	virtual void			FrobHeld(bool frobMaster, bool isFrobPeerAction = false, int holdTime = 0) override;
	virtual void			FrobReleased(bool frobMaster, bool isFrobPeerAction = false, int holdTime = 0) override;

	void					RegisterAI(idAI* ai);	// grayman #1145
	void					TellRegisteredUsers();	// grayman #1145
	idVec3					GetRotationAxis();		// grayman #2691
	idVec3					GetClosedOrigin();		// grayman #2861

	/**
	* This is the non-script version of GetOpen 
	*/
	bool					IsOpen()
	{
		return m_Open;
	}

	/**
	* This indicates if the door was interrupted in its last action
	*/
	bool					WasInterrupted()
	{
		return m_bInterrupted;
	}

	bool					WasStoppedDueToBlock()
	{
		return 	m_StoppedDueToBlock;
	}

	idEntity* 				GetLastBlockingEnt()
	{
		return m_LastBlockingEnt.GetEntity();
	}

	idBox GetClosedBox();			// grayman #2345

	/**
	* This is the non-script version of GetLock
	*/
	virtual bool			IsLocked()
	{
		return m_Lock->IsLocked();
	}

	virtual bool			IsPickable()
	{
		return m_Lock->IsPickable();
	}

	/**
	* Overload the apply impulse function to see if we should change mover
	* state when impulse is applied
	*
	* Description of function from idEntity::ApplyImpulse
	* apply an impulse to the physics object, 'ent' is the entity applying the impulse
	**/

	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) override;

	/**
	* Overloading idMover::DoneRotating/DoneMoving in order to close the portal when door closes
	**/
	virtual void			DoneRotating() override;
	virtual void			DoneMoving() override;

	/**
	 * A helper function that implements the finalisation for rotations or movings.
	 */
	virtual void			DoneStateChange();
	virtual void			CallStateScript();

	/**
	* This is used to test if the mover is moving
	*/
	virtual bool			IsMoving();

	/**
	* This is used to test if it is changing state
	*/
	virtual bool			IsChangingState();

	ID_INLINE const idVec3&	GetOpenPos() const
	{
		return m_OpenPos;
	}

	ID_INLINE const idVec3&	GetClosedPos() const
	{
		return m_ClosedPos;
	}

	ID_INLINE const idVec3&	GetOpenDir() const
	{
		return m_OpenDir;
	}

	ID_INLINE const idAngles& GetClosedAngles() const
	{
		return m_ClosedAngles;
	}

	idVec3 GetCurrentPos();

	ID_INLINE const idVec3& GetTranslation() const
	{
		return m_Translation;
	}

	/** 
	 * greebo: Sets the position of this frobmover based on 
	 * the given float, which should be in the range [0..1].
	 *
	 * @fraction: [0..1] where 0 refers to the original position and 
	 *            a value of 1 refers to the fully rotated handle.
	 */
	void SetFractionalPosition(float fraction, bool immediately);

	float GetFractionalPosition();

	/**
	* This is used to get the remaining translation left on the clip model
	* if it is moving
	*
	* @param out_translation: Passes back out the translation remaining in
	*	the current movement.
	*
	* @param out_deltaAngles: Passes back out the rotation remaining in
	*	the current movement.
	*/
	void GetRemainingMovement(idVec3& out_deltaPosition, idAngles& out_deltaAngles);

	// angua: returns the AAS area the center of the door is located in
	// stgatilov: or -1 if AAS is invalid, or 0 if it does not belong to AAS
	int GetAASArea(idAAS* aas);

	/**
	 * greebo: Within the FrobMover class hierarchy, this function should be used to play sounds 
	 * instead of the standard StartSound() method. Some frobmovers like doors might want to 
	 * relay the sound playing to another entity (like doorhandles) to avoid sounds being
	 * played from the door's origin, barely audible to the player.
	 *
	 * @returns: The length of the sound in msec.
	 */
	virtual int	FrobMoverStartSound(const char* soundName);

	/** 
	 * greebo: Returns TRUE if the mover is at the open position. Doesn't change
	 * the state of the mover, just compares the local angles and origin.
	 */
	virtual bool IsAtOpenPosition();

	/** 
	 * greebo: Returns TRUE if the mover is at the closed position. Doesn't change
	 * the state of the mover, just compares the local angles and origin.
	 */
	virtual bool IsAtClosedPosition();

	/**
	 * grayman #3462 - retrieve the amount of time it takes to move
	 *
	 */
	int GetMoveTime();

	/**
	 * grayman #3755 - set the translation speed
	 *
	 */
	void SetTransSpeed(float speed);

	/**
	 * grayman #3755 - retrieve the translation speed
	 *
	 */
	float GetTransSpeed();

	/**
	 * grayman #3462 - retrieve the time when the mover started to open
	 *
	 */
	int GetMoveStartTime();

	/**
	 * grayman #3643 - Tells the AI where to stand when using the button or switch.
	 *
	 */
	bool GetSwitchGoal(idVec3 &goal, float &standOff, int relightHeightLow);
	//bool GetSwitchGoal(idAI* owner, idVec3 &goal, float &standOff, int relightHeightLow);

	/**
	 * grayman #3643 - Fetch the lock.
	 *
	 */
	PickableLock* GetLock()
	{
		return m_Lock;
	};

	//stgatilov #5683: update members after moving this via hot-reload
	void SetMapOriginAxis(const idVec3 *newOrigin, const idMat3 *newAxis);

protected:

	// ===================== Overridable events ================

	/**
	 * greebo: Gets called when the mover actually starts to move, regardless
	 * what the state was beforehand. The boolean tells which state the mover
	 * is heading towards.
	 */
	virtual void OnMoveStart(bool opening);

	/**
	 * greebo: Is called before the mover is told to open. Based on the return
	 * value of this function, the mover continues its Opening operation or cancels it.
	 *
	 * Subclasses can implement this call to prevent the mover from opening (e.g. when 
	 * a door is locked) or to play sounds before opening starts.
	 *
	 * The analogue function for closing is PreClose(), of course.
	 *
	 * @returns: TRUE if the mover can continue opening/closing, FALSE to cancel the process.
	 */
	virtual bool PreOpen();
	virtual bool PreClose();

	/**
	 * greebo: This is invoked when the mover is receiving an "interrupt request".
	 * If this function returns TRUE, the mover is allowed to be interrupted, 
	 * on FALSE the mover continues its moves and the request is declined.
	 */
	virtual bool PreInterrupt();

	/**
	 * greebo: This is called before the frobmover is locked. When this function
	 * returns TRUE, the mover is allowed to be locked, returning FALSE will
	 * let the mover stay unlocked.
	 */
	virtual bool PreLock(bool bMaster);

	/**
	 * greebo: This is called before the frobmover is unlocked. When this function
	 * returns TRUE, the mover is allowed to be unlocked, returning FALSE will
	 * let the mover stay locked.
	 */
	virtual bool PreUnlock(bool bMaster);

	/**
	 * greebo: Gets called when the mover opens. The boolean tells the function 
	 * whether the mover is starting from its fully closed state.
	 *
	 * This can be implemented by subclasses to do stuff that should be done
	 * when the "fully closed" => "opening" transition is happening, like opening 
	 * visportals or playing open sounds.
	 */
	virtual void OnStartOpen(bool wasClosed, bool bMaster);

	/**
	 * greebo: Gets called when the mover is closing. The boolean tells the function 
	 * whether the mover is starting from its fully "open" state.
	 *
	 * This can be implemented by subclasses to do stuff that should be done
	 * when the "fully open" => "opening" transition is happening.
	 */
	virtual void OnStartClose(bool wasOpen, bool bMaster);

	/**
	 * greebo: Gets called when the mover stops its opening move and has
	 * reached its final open position. This not only gets called when the
	 * mover has been actually moving beforehand.
	 */
	virtual void OnOpenPositionReached();

	/**
	 * greebo: Gets called when the mover finishes its closing move and is fully closed.
	 */
	virtual void OnClosedPositionReached();

	/**
	 * greebo: Is called when the mover has been interrupted (move has already been stopped).
	 */
	virtual void OnInterrupt();

	/**
	 * greebo: Is called when the mover has just been locked.
	 */
	virtual void OnLock(bool bMaster);

	/**
	 * greebo: Is called when the mover has just been unlocked.
	 */
	virtual bool OnUnlock(bool bMaster); // grayman #3643

	// =========================================================

	// An event for convenience. Gets called right after spawn time at time 0.
	// Override this event to do your stuff in the subclass, but be sure to call the baseclass
	virtual void PostSpawn();

	/**
	 * stgatilov #5683: Recompute m_ClosedBox, m_ClosedPos, m_OpenPos, m_OpenDir from basic members.
	 */
	void ComputeAdditionalMembers();

	/** 
	 * greebo: Tells the frobmover to start moving. The boolean specifies whether
	 * to open or to close.
	 *
	 * @returns: TRUE if the mover had something to move or FALSE if the target position
	 * has already been reached.
	 */
	virtual bool StartMoving(bool open);

	/**
	 * greebo: Overrides the base class method to calculate the move_time fraction
	 *         according to the current rotation and translation states. This is needed to let doors
	 *         open/close in the right speed after they've been interrupted.
	 */
	virtual float			GetMoveTimeRotationFraction() override; // grayman #3711
	virtual float			GetMoveTimeTranslationFraction() override; // grayman #3711

	/**
	* By default, a BinaryFrobMover toggles its state when triggered
	**/
	void					Event_Activate( idEntity *activator );

	// Script event to return the locked status
	void					Event_IsLocked();
	void					Event_IsPickable();

	// Script events for opening and closing
	void					Event_Open();
	void					Event_Close();
	void					Event_ToggleOpen();
	void					Event_IsOpen();

	// Script events for locking and unlocking
	void					Event_Lock();
	void					Event_Unlock();
	void					Event_ToggleLock();

	// This is called periodically, to handle a pending close request (used for locking movers after closing)
	void					Event_HandleLockRequest();

	/**
	 * greebo: "Override" the TeamBlocked event to detect collisions with the player.
	 */
	virtual void			OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity) override;

protected:

	// The actual lock implementation for this mover
	PickableLock*				m_Lock;

	/**
	* m_Open is only set to false when the door is completely closed
	**/
	bool						m_Open;

	/**
	* Stores whether the player intends to open or close the door
	* Useful for determining what to do when the door is stopped midway.
	**/
	bool						m_bIntentOpen;

	bool						m_StateChange;

	/**
	* Determines whether the door may be interrupted.  Read from spawnargs.
	**/
	bool						m_bInterruptable;

	/**
	* Set to true if the door was stopped mid-rotation
	**/
	bool						m_bInterrupted;

	/**
	 *	angua: this is set true if the door was interrupted by a blocking object
	 *	the flag of the physics object is cleared after a frame, this one stays true 
	 *	until the door changes its state again
	 */
	bool						m_StoppedDueToBlock;

	int							m_nextBounceTime; // grayman #3755

	idEntityPtr<idEntity>		m_LastBlockingEnt;

	/**
	* Read from the spawnargs, interpreted into m_OpenAngles and m_ClosedAngles
	**/
	idAngles					m_Rotate;

	/**
	 * Original position
	 */
	idVec3						m_StartPos;

	/**
	 * Vector that specifies the direction and length of the translation.
	 * This is needed for doors that don't rotate, but slide to open.
	 */
	idVec3						m_Translation;

	/**
	* Translation speed in doom units per second, read from spawnargs.
	* Defaults to zero.  
	* If set to zero, D3's physics chooses the speed based on a constant time of movement.
	**/
	float						m_TransSpeed;

	/**
	* Door angles when completely closed
	**/
	idAngles					m_ClosedAngles;

	/**
	* Door angles when completely open
	**/
	idAngles					m_OpenAngles;

	// angua: Position of the origin when the door is completely closed or open
	idVec3						m_ClosedOrigin;
	idVec3						m_OpenOrigin;

	// angua: the positions of the far edge measured from the origin 
	// when the door is closed or open.
	idVec3						m_ClosedPos;
	idVec3						m_OpenPos;

	// angua: points toward the opening direction of the door 
	// (perpendicular to door face and rotation axis)
	idVec3						m_OpenDir;

	/**
	 * Scriptfunction that is called, whenever the door is finished rotating
	 * or translating. i.E. when the statechange is completed.
	 * The function gets as parameters the current state:
	 * DoorComplete(boolean open, boolean locked, boolean interrupted);
	 */
	idStr						m_CompletionScript;

	bool						m_Rotating;
	bool						m_Translating;

	/**
	* The following variables determine the behavior when the binary mover
	* receives an impulse from the physics system.
	*
	* Square of the impulse magnitude thresholds for "opening" and "closing"
	* If these are set to 0 or below, it does nothing.
	**/
	float						m_ImpulseThreshCloseSq;
	float						m_ImpulseThreshOpenSq;

	/**
	* Normalized direction vectors to resolve the impulse along
	* when opening and closing.
	* I.e., the impulse vector * this axis yields the impulse magnitude 
	* that is compared to the threshold.
	*
	* (NOTE: These default to zero, and if they are set to zero,
	* the axis of the impulse is not checked, impulse magnitude is used in comparison)
	**/
	idVec3						m_vImpulseDirOpen;
	idVec3						m_vImpulseDirClose;

	/** 
	 * greebo: If this is TRUE, the door will stop as soon as something is in the way.
	 *         Corresponds to spawnarg "stop_when_blocked"
	 */
	bool						m_stopWhenBlocked;

	/**
	 * greebo: This is set to TRUE when the mover should be locked as soon as it has
	 * reached its closed position.
	 */
	bool						m_LockOnClose;

	/**
	* Ishtvan: Used for fine control of opening/closing with the mouse
	**/
	idVec2						m_mousePosition;

	/**
	* True when frob is held down but not long enough to initialize fine control
	**/
	bool						m_bFineControlStarting;

	/**
	* grayman #2345 - idBox of the closed mover, used in pathfinding
	**/

	idBox						m_closedBox;

	/**
	* grayman #1145 - list of AI who unsuccessfully tried a locked door
	**/

	idList< idEntityPtr<idAI> >	m_registeredAI;

	/**
	* grayman #2859 - the last AI to open or close this door, otherwise NULL
	**/

	idEntityPtr<idEntity>		m_lastUsedBy;

	/**
	* grayman #1327 - an AI that's searching around the door
	**/

	idEntityPtr<idEntity>		m_searching;

	/**
	* grayman #3029 - temporarily turn off targeting for elevator fetch buttons
	**/

	bool						m_targetingOff;

	/**
	* grayman #3104 - when an AI needs to use this door, was it locked?
	**/

	bool						m_wasFoundLocked;

	/**
	* grayman #3462 - when the door started moving
	**/
	int							m_timeDoorStartedMoving;
};

#endif /* !BINARYFROBMOVER */
