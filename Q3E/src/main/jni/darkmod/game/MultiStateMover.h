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
#ifndef _MULTI_STATE_MOVER_H_
#define _MULTI_STATE_MOVER_H_

#include "MultiStateMoverPosition.h"
#include "MultiStateMoverButton.h"

/**
 * greebo: A MultiState mover is an extension to the vanilla D3 movers.
 *
 * In contrast to the idElevator class, this multistate mover draws the floor
 * information from the MultiStateMoverPosition entities which are targetted
 * by this class.
 * 
 * The MultiStateMover will start moving when it's triggered (e.g. by buttons), 
 * where the information where to go is contained on the triggering button.
 *
 * Furthermore, the MultiStateMover provides a public interface for AI to 
 * help them figuring out which buttons to use, where to go, etc.
 */
class CMultiStateMover : 
	public idMover
{
private:
	idList<MoverPositionInfo> positionInfo;

	idVec3 forwardDirection;

	// The lists of buttons, AI entities need them to get the elevator moving
	idList< idEntityPtr<CMultiStateMoverButton> > fetchButtons;
	idList< idEntityPtr<CMultiStateMoverButton> > rideButtons;

	// grayman #3050 - to help reduce jostling on the elevator
	bool masterAtRideButton; // is the master in position to push the ride button?
	UserManager riderManager; // manage a list of riders (which is different than users)

public:
	CLASS_PROTOTYPE( CMultiStateMover );

	CMultiStateMover();

	void	Spawn();

	void	Save(idSaveGame *savefile) const;
	void	Restore(idRestoreGame *savefile);

	virtual void Activate(idEntity* activator) override;

	// Returns the list of position infos, populates the list if none are assigned yet.
	const idList<MoverPositionInfo>& GetPositionInfoList();

	/** 
	 * greebo: Returns TRUE if this mover is at the given position.
	 * Note: NULL arguments always return false.
	 */
	bool IsAtPosition(CMultiStateMoverPosition* position);

	/** 
	 * greebo: This is called by the MultiStateMoverButton class exclusively to
	 * register the button with this Mover, so that the mover knows which buttons
	 * can be used to get it moving.
	 *
	 * @button: The button entity
	 * @type: The "use type" of the entity, e.g. "fetch" or "ride"
	 */
	void RegisterButton(CMultiStateMoverButton* button, EMMButtonType type);

	/** 
	 * greebo: Returns the closest button entity for the given position and the given "use type".
	 *
	 * @toPosition: the position the elevator needs to go to (to be "fetched" to, to "ride" to).
	 * @fromPosition: the position the button needs to be accessed from (can be NULL for type == RIDE).
	 * @type: the desired type of button (fetch or ride)
	 * @riderOrg: the origin of the AI using the button (grayman #3029)
	 * 
	 * @returns: NULL if nothing found.
	 */
	CMultiStateMoverButton* GetButton(CMultiStateMoverPosition* toPosition, CMultiStateMoverPosition* fromPosition, EMMButtonType type, idVec3 riderOrg); // grayman #3029

	// grayman #3050 - for ride management
	bool IsMasterAtRideButton();
	void SetMasterAtRideButton(bool atButton);
	inline UserManager& GetRiderManager()
	{
		return riderManager;
	}

protected:
	// override idMover's DoneMoving() to trigger targets
	virtual void DoneMoving() override;

private:
	// greebo: Controls the direction of targetted rotaters, depending on the given moveTargetPosition
	void	SetGearDirection(const idVec3& targetPos);

	// Returns the index of the named position info or -1 if not found
	int		GetPositionInfoIndex(const idStr& name) const;

	// Returns the index of the position info at the given position (using epsilon comparison)
	int		GetPositionInfoIndex(const idVec3& pos) const;

	// Returns the positioninfo entity of the given location or NULL if no suitable position found 
	CMultiStateMoverPosition* GetPositionEntity(const idVec3& pos) const;

	// Extracts all position entities from the targets
	void FindPositionEntities();

	void Event_Activate(idEntity* activator);
	void Event_PostSpawn();

	/**
	 * grayman #4370: "Override" the TeamBlocked event to detect collisions with the player.
	 */
	virtual void OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity) override;
};

#endif /* _MULTI_STATE_MOVER_H_ */
