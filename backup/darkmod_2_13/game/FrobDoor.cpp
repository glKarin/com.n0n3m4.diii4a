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
// Copyright (C) 2004-2011 The Dark Mod Team

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/Category.h"
#include "FrobDoor.h"
#include "FrobDoorHandle.h"
#include "../tools/compilers/aas/AASFile.h"
#include "SndProp.h"
#include "StimResponse/StimResponseTimer.h"
#include "StimResponse/StimResponse.h"

#include "../game/ai/AAS.h"

#define EXTRA_PLAYER_LOSS 9.0f // grayman #3042
#define FRACTION_OPEN_FOR_MAX_VOL 0.50f // grayman #3042 - fraction open at which max
										// sound volume comes through an opening door
#define DOOR_SPEED_RATE_INCREASE_MIN 7168.0f  // grayman #3755
#define DOOR_SPEED_RATE_INCREASE_MAX 10752.0f // grayman #3755


//===============================================================================
//CFrobDoor
//===============================================================================

const idEventDef EV_TDM_Door_OpenDoor( "OpenDoor", EventArgs('f', "master", ""), EV_RETURNS_VOID, 
	"The OpenDoor method is necessary to give the FrobDoorHandles a \n" \
	"\"low level\" open routine. The CFrobDoor::Open() call is re-routed to\n" \
	"the FrobDoorHandle::Tap() method, so there must be a way to actually\n" \
	"let the door open. Which is what this method does.\n" \
	"\n" \
	"Note: Shouldn't be called directly by scripters, call handle->Tap() instead.\n" \
	"Unless you know what you're doing.");
const idEventDef EV_TDM_Door_GetDoorhandle( "GetDoorhandle", EventArgs(), 'e', "Returns the handle entity of this door. Can return NULL (== $null_entity)" );

const idEventDef EV_TDM_Door_ClearPlayerImmobilization("_EV_TDM_Door_ClearPlayerImmobilization", 
	EventArgs('e', "", ""), EV_RETURNS_VOID, "internal"); // allows player to handle weapons again

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoor )
	EVENT( EV_TDM_Door_OpenDoor,					CFrobDoor::Event_OpenDoor)
	EVENT( EV_TDM_FrobMover_HandleLockRequest,		CFrobDoor::Event_HandleLockRequest ) // overrides binaryfrobmover's request
	EVENT( EV_TDM_Door_GetDoorhandle,				CFrobDoor::Event_GetDoorhandle)

	// Needed for PickableLock: Update Handle position on lockpick status update
	EVENT( EV_TDM_Lock_StatusUpdate,				CFrobDoor::Event_Lock_StatusUpdate)
	EVENT( EV_TDM_Lock_OnLockPicked,				CFrobDoor::Event_Lock_OnLockPicked)
	EVENT( EV_TDM_Door_ClearPlayerImmobilization,	CFrobDoor::Event_ClearPlayerImmobilization )
	EVENT( EV_PostPostSpawn,						CFrobDoor::Event_PostPostSpawn ) // grayman #3643
END_CLASS

CFrobDoor::CFrobDoor()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_door";
	m_DoubleDoor = NULL;
	m_LastHandleUpdateTime = -1;

	// grayman #3042 - sound loss variables
	m_lossOpen = 0;
	m_lossDoubleOpen = 0;
	m_lossClosed = 0;
	m_lossBaseAI = 0;			// AI sound loss provided by other entities, i.e. location separator
	m_lossBasePlayer = 0;		// Player sound loss provided by other entities, i.e. location separator
	m_isTransparent = false;
	m_controllers.Clear();		// grayman #3643 - list of my controllers
	m_peek = NULL;			// grayman #4882 - my peek entity
	m_doorHandlingPositions.Clear(); // grayman #3643 - list of my door handling positions
	m_AIPushingDoor = false;	// grayman #3748
	m_speedFactor = -1.0f;
	m_previouslyFrobable = false;
	m_previouslyPushingPlayer = false;
	m_rotates = false;
	memset(m_doorPositions, 0, sizeof(m_doorPositions));
}

CFrobDoor::~CFrobDoor()
{
	ClearDoorTravelFlag();
}

void CFrobDoor::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(m_OpenPeers.Num());
	for (int i = 0; i < m_OpenPeers.Num(); i++)
	{
		savefile->WriteString(m_OpenPeers[i]);
	}

	savefile->WriteInt(m_LockPeers.Num());
	for (int i = 0; i < m_LockPeers.Num(); i++)
	{
		savefile->WriteString(m_LockPeers[i]);
	}

	m_DoubleDoor.Save(savefile);

	savefile->WriteInt(m_Doorhandles.Num());
	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		m_Doorhandles[i].Save(savefile);
	}

	savefile->WriteInt(m_LastHandleUpdateTime);

	// grayman #3042 - sound loss variables
	savefile->WriteFloat(m_lossOpen);
	savefile->WriteFloat(m_lossDoubleOpen);
	savefile->WriteFloat(m_lossClosed);
	savefile->WriteFloat(m_lossBaseAI);
	savefile->WriteFloat(m_lossBasePlayer);

	savefile->WriteBool(m_isTransparent);	// grayman #3042
	savefile->WriteBool(m_rotates);			// grayman #3643
	savefile->WriteBool(m_previouslyPushingPlayer); // grayman #3748
	savefile->WriteBool(m_previouslyFrobable); // grayman #3748
	savefile->WriteBool(m_AIPushingDoor);	// grayman #3748
	savefile->WriteFloat(m_speedFactor);	// grayman #3755

	// grayman #3643 - door-handling positions
	for ( int i = 0 ; i < DOOR_SIDES ; i++ )
	{
		for ( int j = 0 ; j < NUM_DOOR_POSITIONS ; j++ )
		{
			savefile->WriteVec3(m_doorPositions[i][j]);
		}
	}

	// grayman #3643 - list of door controllers
	savefile->WriteInt(m_controllers.Num());
	for ( int i = 0 ; i < m_controllers.Num() ; i++ )
	{
		m_controllers[i].Save(savefile);
	}

	// grayman #4882 - peek entity
	m_peek.Save(savefile);

	// grayman #3643 - list of door handling positions
	savefile->WriteInt(m_doorHandlingPositions.Num());
	for ( int i = 0 ; i < m_doorHandlingPositions.Num() ; i++ )
	{
		m_doorHandlingPositions[i].Save(savefile);
	}
}

void CFrobDoor::Restore( idRestoreGame *savefile )
{
	int num;
	savefile->ReadInt(num);
	m_OpenPeers.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(m_OpenPeers[i]);
	}

	savefile->ReadInt(num);
	m_LockPeers.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(m_LockPeers[i]);
	}

	m_DoubleDoor.Restore(savefile);
	
	savefile->ReadInt(num);
	m_Doorhandles.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_Doorhandles[i].Restore(savefile);
	}

	savefile->ReadInt(m_LastHandleUpdateTime);

	// grayman #3042 - sound loss variables
	savefile->ReadFloat(m_lossOpen);	
	savefile->ReadFloat(m_lossDoubleOpen);
	savefile->ReadFloat(m_lossClosed);
	savefile->ReadFloat(m_lossBaseAI);
	savefile->ReadFloat(m_lossBasePlayer);

	savefile->ReadBool(m_isTransparent);	// grayman #3042
	savefile->ReadBool(m_rotates);			// grayman #3643
	savefile->ReadBool(m_previouslyPushingPlayer); // grayman #3748
	savefile->ReadBool(m_previouslyFrobable); // grayman #3748
	savefile->ReadBool(m_AIPushingDoor);	// grayman #3748
	savefile->ReadFloat(m_speedFactor);		// grayman #3755

	// grayman #3643 - door-handling positions
	for ( int i = 0 ; i < DOOR_SIDES ; i++ )
	{
		for ( int j = 0 ; j < NUM_DOOR_POSITIONS ; j++ )
		{
			savefile->ReadVec3(m_doorPositions[i][j]);
		}
	}

	// grayman #3643 - list of door controllers
	m_controllers.Clear();
	savefile->ReadInt(num);
	m_controllers.SetNum(num);
	for ( int i = 0 ; i < num ; i++ )
	{
		m_controllers[i].Restore(savefile);
	}

	// grayman #4882 - peek entity
	m_peek.Restore(savefile);

	// grayman #3643 - list of door handling positions
	m_doorHandlingPositions.Clear();
	savefile->ReadInt(num);
	m_doorHandlingPositions.SetNum(num);
	for ( int i = 0 ; i < num ; i++ )
	{
		m_doorHandlingPositions[i].Restore(savefile);
	}

	SetDoorTravelFlag();

	UpdateSoundLoss(); // grayman #3455
}

void CFrobDoor::Spawn()
{
}

void CFrobDoor::PostSpawn()
{
	// Let the base class do its stuff first
	CBinaryFrobMover::PostSpawn();

	// Locate the double door entity before closing our portal
	FindDoubleDoor();

	// grayman #3042 - obtain sound loss values
	m_lossOpen = spawnArgs.GetFloat("loss_open", "1.0");
	m_lossDoubleOpen = spawnArgs.GetFloat("loss_double_open", "1.0");
	m_lossClosed = spawnArgs.GetFloat("loss_closed", "10.0");

	// grayman #3042 - does this door contain a transparent texture, or have openings?
	m_isTransparent = spawnArgs.GetBool("transparent","0");

	// Wait until here for the first update of sound loss, in case a double door is open
	UpdateSoundLoss();

	if (!m_Open)
	{
		// Door starts _completely_ closed, try to shut the portal.
		ClosePortal();
	}

	// Open the portal if either of the doors is open
	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();
	if (m_Open || ((doubleDoor != NULL) && doubleDoor->IsOpen()))
	{
		OpenPortal();
	}

	// Search for all spawnargs matching "open_peer_N" and add them to our open peer list
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("open_peer"); kv != NULL; kv = spawnArgs.MatchPrefix("open_peer", kv))
	{
		// Add the peer
		AddOpenPeer(kv->GetValue());
	}

	// Search for all spawnargs matching "lock_peer_N" and add them to our lock peer list
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("lock_peer"); kv != NULL; kv = spawnArgs.MatchPrefix("lock_peer", kv))
	{
		// Add the peer
		AddLockPeer(kv->GetValue());
	}

	// grayman #3643 - Search for all spawnargs matching "door_controller" and add the entities to our controller list
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("door_controller"); kv != NULL; kv = spawnArgs.MatchPrefix("door_controller", kv))
	{
		idStr str = kv->GetValue();
		idEntity* doorController = gameLocal.FindEntity(str);

		if ( (doorController != NULL) && doorController->IsType(CBinaryFrobMover::Type) )
		{
			AddController(doorController);
		}
	}
		
	// grayman #3643 - Search for all spawnargs matching "door_handle_position" and add the entities to our door handle position list
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("door_handle_position"); kv != NULL; kv = spawnArgs.MatchPrefix("door_handle_position", kv))
	{
		idStr str = kv->GetValue();
		idEntity* dhp = gameLocal.FindEntity(str);

		if (dhp)
		{
			const char *classname;
			dhp->spawnArgs.GetString("classname", NULL, &classname);
			if (idStr::Cmp(classname, "atdm:door_handling_position") == 0)
			{
				AddDHPosition(dhp);
			}
		}
	}
		
	idStr doorHandleName = spawnArgs.GetString("door_handle", "");
	if (!doorHandleName.IsEmpty())
	{
		idEntity* handleEnt = gameLocal.FindEntity(doorHandleName);

		if (handleEnt != NULL && handleEnt->IsType(CFrobDoorHandle::Type))
		{
			// Convert to frobdoorhandle pointer and call the helper function
			CFrobDoorHandle* handle = static_cast<CFrobDoorHandle*>(handleEnt);
			AddDoorhandle(handle);

			// Check if we should bind the named handle to ourselves
			if (spawnArgs.GetBool("door_handle_bind_flag", "1"))
			{
				handle->Bind(this, true);
			}
		}
		else
		{
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Doorhandle entity not spawned or of wrong type: %s\r", doorHandleName.c_str());
		}
	}

	// greebo: Should we auto-setup the doorhandles?
	if (spawnArgs.GetBool("auto_setup_door_handles", "1"))
	{
		AutoSetupDoorHandles();
	}

	// grayman #4882 - set up peek entity if present
	idList<idEntity *> children;
	GetTeamChildren(&children);
	m_peek = NULL;

	for ( int i = 0; i < children.Num(); i++ )
	{
		if ( children[i]->IsType(idPeek::Type) )
		{
			idPeek *peekEntity = static_cast<idPeek*>(children[i]);
			m_peek = peekEntity;
			break;
		}
	}

	// greebo: Should we auto-setup the double door behaviour?
	if (spawnArgs.GetBool("auto_setup_double_door", "1"))
	{
		AutoSetupDoubleDoor();
	}

	SetDoorTravelFlag();

	// grayman #3755 - set move speed factor for AI in a hurry

	idBox closedBox = GetClosedBox();
	idVec3 extents = closedBox.GetExtents();
	float area;
	area = (2*extents.z) * (2*((extents.x > extents.y) ? extents.x : extents.y));
	if (area <= DOOR_SPEED_RATE_INCREASE_MIN)
	{
		m_speedFactor = 2.0f;
	}
	else if (area >= DOOR_SPEED_RATE_INCREASE_MAX)
	{
		m_speedFactor = 1.0f;
	}
	else
	{
		m_speedFactor = 1.0f + (DOOR_SPEED_RATE_INCREASE_MAX - area)/(DOOR_SPEED_RATE_INCREASE_MAX - DOOR_SPEED_RATE_INCREASE_MIN);
	}

	// grayman #3643 - Wait a bit before setting the door handling
	// positions, to allow time for any controller list to be built
	// after the controllers have spawned

	PostEventMS( &EV_PostPostSpawn, 2 );
}

void CFrobDoor::Event_PostPostSpawn()
{
	GetDoorHandlingPositions(); // grayman #3643
	SetControllerLocks();
}

void CFrobDoor::SetDoorTravelFlag()
{
	// Flag the AAS areas the door is located in with door travel flag
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS*	aas = gameLocal.GetAAS(i);
		if (aas == NULL)
		{
			continue;
		}
		
		int areaNum = GetAASArea(aas);
		aas->SetAreaTravelFlag(areaNum, TFL_DOOR);
		aas->ReferenceDoor(this, areaNum);
	}
}

void CFrobDoor::ClearDoorTravelFlag()
{
	bool valid_aas = false;

	// Remove the door travel flag from the AAS areas the door is located in
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS*	aas = gameLocal.GetAAS(i);
		if (aas == NULL)
		{
			continue;
		}
		
		int areaNum = GetAASArea(aas);
		if (areaNum > 0)
		{
			aas->RemoveAreaTravelFlag(areaNum, TFL_DOOR);
			aas->DeReferenceDoor(this, areaNum);
			// Found a valid area, supress the warning below
			valid_aas = true;
		}
	}
	/*if (!valid_aas)
	{
		gameLocal.Warning("Door %s is not within a valid AAS area", name.c_str());
	}*/
}

void CFrobDoor::Lock(bool bMaster)
{
	// Pass the call to the base class, the OnLock() event will be fired 
	// if the locking process is allowed
	CBinaryFrobMover::Lock(bMaster);
}

void CFrobDoor::OnLock(bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnLock(bMaster);

	if (bMaster)
	{
		LockPeers();
	}
}

// grayman #3643 - return 'true' if unlock automatically opens the door, 'false' if not
bool CFrobDoor::Unlock(bool bMaster)
{
	// Pass the call to the base class, the OnUnlock() event will be fired 
	// if the locking process is allowed
	return (CBinaryFrobMover::Unlock(bMaster));
}

// grayman #3643 - report back whether OnUnlock() automatically opens the door
bool CFrobDoor::OnUnlock(bool bMaster)
{
	// Call the base class first
	bool result = CBinaryFrobMover::OnUnlock(bMaster);

	if (bMaster) 
	{
		UnlockPeers();
	}

	return result;
}

void CFrobDoor::Open(bool bMaster)
{
	// If we have a doorhandle we want to tap it before the door starts to open if the door wasn't
	// already interrupted
	if (m_Doorhandles.Num() > 0 && !m_bInterrupted)
	{
		// Relay the call to the handles, the OpenDoor() call will come back to us
		for (int i = 0; i < m_Doorhandles.Num(); i++)
		{
			CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
			if (handle == NULL)
			{
				continue;
			}

			handle->Tap();
		}

		if (bMaster)
		{
			TapPeers();
		}
	}
	else
	{
		// No handle present or interrupted, let's just proceed with our own open routine
		OpenDoor(bMaster);
	}
}

void CFrobDoor::OpenDoor(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opening\r" );

	// Now pass the call to the base class, which will invoke PreOpen() and the other events
	CBinaryFrobMover::Open(bMaster);
}

void CFrobDoor::OnStartOpen(bool wasClosed, bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnStartOpen(wasClosed, bMaster);

	// We are actually opening, open the visportal too
	OpenPortal();

	// Update soundprop
	UpdateSoundLoss();

	if (bMaster)
	{
		OpenPeers();
	}
}

void CFrobDoor::Close(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closing\r" );

	// Invoke the close method in any case
	CBinaryFrobMover::Close(bMaster);
}

void CFrobDoor::OnStartClose(bool wasOpen, bool bMaster)
{
	CBinaryFrobMover::OnStartClose(wasOpen, bMaster);

	if (bMaster)
	{
		ClosePeers();
	}
}

void CFrobDoor::OnClosedPositionReached() 
{
	// Call the base class
	CBinaryFrobMover::OnClosedPositionReached();

	// Try to close the visportal
	ClosePortal();

	// Update the sound propagation values
	UpdateSoundLoss();

	// grayman #2866 - clear who used it last
	SetLastUsedBy( NULL );
}

bool CFrobDoor::CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) 
{
	// First, check if the frob master can be used
	// If this doesn't succeed, perform additional checks
	idEntity* master = GetFrobMaster();
	if( master != NULL && master->CanBeUsedByItem(item, isFrobUse) )
	{
		return true;
	}

	if (item == NULL)
	{
		return false;
	}

	assert(item->Category() != NULL);

	// FIXME: Move this to idEntity to some sort of "usable_by_inv_category" list?
	const idStr& itemName = item->Category()->GetName();
	if (itemName == "#str_02392" )						// Keys
	{
		// Keys can always be used on doors
		// Exception: for "frob use" this only applies when the door is locked
		return (isFrobUse) ? IsLocked() : true;
	}
	else if (itemName == "#str_02389" )					// Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks behave similar to keys
		return (isFrobUse) ? IsLocked() : true;
	}

	return false;
}

bool CFrobDoor::UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	if (item == NULL)
	{
		return false;
	}

	// Pass the call on to the master, if we have one
	if (GetFrobMaster() != NULL) 
	{
		return GetFrobMaster()->UseByItem(impulseState, item);
	}

	// Call the used_action_script via the generic idEntity method, if desired by the mapper
	if ( spawnArgs.GetBool("call_used_action_script", "0") )
	{
		idEntity::UseByItem(impulseState, item);
	}

	assert(item->Category() != NULL);

	// Retrieve the entity behind that item and reject NULL entities
	idEntity* itemEntity = item->GetItemEntity();
	if (itemEntity == NULL)
	{
		return false;
	}

	// Get the name of this inventory category
	const idStr& itemName = item->Category()->GetName();
	
	if (itemName == "#str_02392" && impulseState == EPressed ) // Keys
	{
		// Keys can be used on button PRESS event, let's see if the key matches
		if (m_UsedByName.FindIndex(itemEntity->name) != -1)
		{
			// If we're locked or closed, just toggle the lock. 
			if (IsLocked() || IsAtClosedPosition())
			{
				ToggleLock();
			}
			// If we're open, set a lock request and start closing.
			else
			{
				// Close the door and set the lock request to true
				CloseAndLock();
			}

			return true;
		}
		else
		{
			FrobMoverStartSound("snd_wrong_key");
			return false;
		}
	}
	else if (itemName == "#str_02389" ) // Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks are different, we need to look at the button state
		// First we check if this item is a lockpick. It has to be of the toolclass lockpick
		// and the type must be set.
		idStr str = itemEntity->spawnArgs.GetString("lockpick_type", "");

		if (str.Length() == 1)
		{
			// greebo: Check if the item owner is a player, and if yes, 
			// update the immobilization flags.
			idEntity* itemOwner = item->GetOwner();

			if (itemOwner->IsType(idPlayer::Type))
			{
				idPlayer* playerOwner = static_cast<idPlayer*>(itemOwner);
				playerOwner->SetImmobilization("Lockpicking", EIM_ATTACK);

				// Schedule an event 1/3 sec. from now, to enable weapons again after this time
				CancelEvents(&EV_TDM_Door_ClearPlayerImmobilization);
				PostEventMS(&EV_TDM_Door_ClearPlayerImmobilization, 300, playerOwner);
			}

			// Pass the call to the lockpick routine
			return m_Lock->ProcessLockpickImpulse(impulseState, static_cast<int>(str[0]));
		}
		else
		{
			gameLocal.Warning("Wrong 'type' spawnarg for lockpicking on item %s, must be a single character.", itemEntity->name.c_str());
			return false;
		}
	}

	return false;
}

void CFrobDoor::UpdateSoundLoss()
{
	if (!areaPortal)
	{
		return; // not a portal door
	}

	CFrobDoor* doorB = m_DoubleDoor.GetEntity();

	// grayman #3042 - add extra loss to loss_closed on doors, but only for the player,
	// to bring sound volume across doors back to what the player used to hear

	float loss_AI = 0;
	float loss_Player = 0;
	bool  AisOpen = IsOpen();

	float A_fractionOpen = GetFractionalPosition();
	float fA = 1.0 - A_fractionOpen/FRACTION_OPEN_FOR_MAX_VOL;
	if ( fA < 0 )
	{
		fA = 0;
	}

	float A_lossOpen = m_lossOpen;
	float A_lossDoubleOpen = m_lossDoubleOpen;
	float A_lossClosed = m_lossClosed;
	
	if ( doorB )
	{
		float B_lossOpen = doorB->m_lossOpen;
		float B_lossDoubleOpen = doorB->m_lossDoubleOpen;
		float B_lossClosed = doorB->m_lossClosed;

		float B_fractionOpen = doorB->GetFractionalPosition();
		float fB = 1.0 - B_fractionOpen/FRACTION_OPEN_FOR_MAX_VOL;
		if ( fB < 0 )
		{
			fB = 0;
		}

		bool BisOpen = doorB->IsOpen();

		if ( AisOpen )
		{
			if ( BisOpen ) // A and B both open
			{
				// AI

				float loss_B = B_lossOpen + ( B_lossClosed - B_lossOpen )*fB;
				loss_AI = A_lossOpen + ( A_lossClosed - A_lossOpen )*fA; // fractional loss from A
				if ( loss_B < loss_AI )
				{
					loss_AI = loss_B;
				}

				// Player
				A_lossClosed += EXTRA_PLAYER_LOSS;
				B_lossClosed += EXTRA_PLAYER_LOSS;

				loss_B = B_lossOpen + ( B_lossClosed - B_lossOpen )*fB;
				loss_Player = A_lossOpen + ( A_lossClosed - A_lossOpen )*fA; // fractional loss from A
				if ( loss_B < loss_Player )
				{
					loss_Player = loss_B;
				}
			}
			else // A open, B closed
			{
				// AI
				loss_AI = A_lossDoubleOpen + ( A_lossClosed - A_lossDoubleOpen )*fA;

				// Player
				A_lossClosed += EXTRA_PLAYER_LOSS;
				loss_Player = A_lossDoubleOpen + ( A_lossClosed - A_lossDoubleOpen )*fA;
			}
		}
		else
		{
			if ( BisOpen ) // A closed, B open
			{
				// AI
				loss_AI = B_lossDoubleOpen + ( B_lossClosed - B_lossDoubleOpen )*fB;

				// Player
				B_lossClosed += EXTRA_PLAYER_LOSS;
				loss_Player = B_lossDoubleOpen + ( B_lossClosed - B_lossDoubleOpen )*fB;
			}
			else // A and B both closed
			{
				// AI
				loss_AI = B_lossClosed;
				if ( A_lossClosed > loss_AI )
				{
					loss_AI = A_lossClosed;
				}

				// Player
				loss_Player = loss_AI + EXTRA_PLAYER_LOSS;
			}
		}
	}
	else // no double door
	{
		if ( AisOpen )
		{
			// AI
			loss_AI = A_lossOpen + ( A_lossClosed - A_lossOpen )*fA;

			// Player
			A_lossClosed += EXTRA_PLAYER_LOSS;
			loss_Player = A_lossOpen + ( A_lossClosed - A_lossOpen )*fA;
		}
		else // A is closed
		{
			loss_AI = A_lossClosed;
			loss_Player = A_lossClosed + EXTRA_PLAYER_LOSS;
		}
	}

	// Add loss from other entities, if present (i.e. location separators)

	// grayman #3042 - this comment was here, and I left it,
	// though I don't know who made it or what it means. AFAIK,
	// there's no spawnarg called sound_char.

	// TODO: check the spawnarg: sound_char, and return the 
	// appropriate loss for that door, open or closed

	// grayman #3042 - allow a base loss value for AI and one for Player
	gameLocal.m_sndProp->SetPortalAILoss(areaPortal, loss_AI + m_lossBaseAI);
	gameLocal.m_sndProp->SetPortalPlayerLoss(areaPortal, loss_Player + m_lossBasePlayer);
}

void CFrobDoor::FindDoubleDoor()
{
	if ( m_DoubleDoor.GetEntity() != NULL )
		return;

#if 1 // grayman 5109
	/* grayman 5109 - Rather than rely on dmap's buggy clipmodel setup for sliding
	   doors whose origins aren't near each other, look through all doors and see if
	   the portal for a door matches the portal for this door. Ignore yourself. If so,
	   make that door the double door for this door, and this door the double door for
	   that one. If someone fixes dmap's clipmodel bug, it won't affect this new approach,
	   which doesn't rely on clipmodels.
	*/

	for( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if ( !ent )
		{
			continue;	// skip nulls
		}

		if ( ent == this ) // skip yourself
		{
			continue;
		}

		if ( ent->IsType(CFrobDoor::Type) )
		{
			CFrobDoor* doubleDoor = static_cast<CFrobDoor*>(ent);

			if ( doubleDoor->m_DoubleDoor.GetEntity() != NULL )
			{
				continue;
			}

			int portal = gameRenderWorld->FindPortal(doubleDoor->GetPhysics()->GetAbsBounds());
			if (( portal != 0 ) && (portal == areaPortal))
			{
				m_DoubleDoor = static_cast<CFrobDoor *>(ent); // I'm paired with my double
				doubleDoor->m_DoubleDoor = this; // My double is paired with me
				break; // grayman #3042
			}
		}
	}
	// end of new code
#else // old code
	idClip_ClipModelList clipModelList;

	idBounds clipBounds = physicsObj.GetAbsBounds();
	clipBounds.ExpandSelf( 10.0f );

	int numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, CONTENTS_SOLID, clipModelList );

	for ( int i = 0 ; i < numListedClipModels ; i++ )
	{
		idClipModel* clipModel = clipModelList[i];
		idEntity* obEnt = clipModel->GetEntity();

		// Ignore self
		if ( obEnt == this )
		{
			continue;
		}

		if ( obEnt->IsType(CFrobDoor::Type) )
		{
				// check the visportal inside the other door, if it's the same as this one => double door
			int testPortal = gameRenderWorld->FindPortal(obEnt->GetPhysics()->GetAbsBounds());

			if ( ( testPortal == areaPortal ) && ( testPortal != 0 ) )
			{
				m_DoubleDoor = static_cast<CFrobDoor*>(obEnt);
				break; // grayman #3042
			}
		}
	}
#endif // end of old code
}

CFrobDoor* CFrobDoor::GetDoubleDoor()
{
	return m_DoubleDoor.GetEntity();
}

int CFrobDoor::GetControllerNumber() // grayman 5109
{
	return m_controllers.Num();
}

void CFrobDoor::SetLossBase( float lossAI, float lossPlayer ) // grayman #3042
{
	m_lossBaseAI = lossAI;
	m_lossBasePlayer = lossPlayer;
}

void CFrobDoor::ClosePortal()
{
	// grayman #3042 - don't close if the door is marked "transparent"
	if ( m_isTransparent )
	{
		return;
	}

	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();

	if (doubleDoor == NULL || !doubleDoor->IsOpen())
	{
		// No double door or double door is closed too
		if (areaPortal) 
		{
			SetPortalState(false);
		}
	}
}

void CFrobDoor::SetFrobbed(const bool val)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("door_body [%s] %08lX is frobbed\r", name.c_str(), this);

	// First invoke the base class, then check for a doorhandle
	idEntity::SetFrobbed(val);

	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
		if (handle == NULL) continue;

		handle->SetFrobbed(val);
	}

	m_Lock->OnFrobbedStatusChange(val);
}

bool CFrobDoor::IsFrobbed() const
{
	// If the door has a handle and it is frobbed, then we are also considered 
	// to be frobbed.
	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
		if (handle == NULL) continue;

		if (handle->IsFrobbed())
		{
			return true;
		}
	}

	// None of the handles are frobbed
	return idEntity::IsFrobbed();
}

void CFrobDoor::UpdateHandlePosition()
{
	// greebo: Don't issue an handle update position call each frame,
	// this might cause movers to freeze in place, as the extrapolation class
	// let's them rest for the first frame
	if (gameLocal.time <= m_LastHandleUpdateTime + USERCMD_MSEC) return;

	m_LastHandleUpdateTime = gameLocal.time;

	// Calculate the fraction based on the current pin/sample state
	float fraction = m_Lock->CalculateHandleMoveFraction();

	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText3", idStr(fraction));
	}

	// Tell the doorhandles to update their position
	for (int i = 0; i < m_Doorhandles.Num(); ++i)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();

		if (handle == NULL) continue;

		handle->SetFractionalPosition(fraction, false);
	}
}

void CFrobDoor::Event_Lock_StatusUpdate()
{
	UpdateHandlePosition();
}

void CFrobDoor::Event_Lock_OnLockPicked()
{
	// "Lock is picked" signal, unlock in master mode
	Unlock(true);
}

void CFrobDoor::AttackAction(idPlayer* player)
{
	idEntity* master = GetFrobMaster();

	if (master != NULL) 
	{
		master->AttackAction(player);
		return;
	}

	// No master
	m_Lock->AttackAction(player);
}

void CFrobDoor::OpenPeers()
{
	OpenClosePeers(true);
}

void CFrobDoor::ClosePeers()
{
	OpenClosePeers(false);
}

void CFrobDoor::OpenClosePeers(const bool open)
{
	// Cycle through our "open peers" list and issue the call to them
	for (int i = 0; i < m_OpenPeers.Num(); i++)
	{
		const idStr& openPeer = m_OpenPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", openPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(openPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for OpenClose\r", openPeer.c_str());

			CFrobDoor* door = static_cast<CFrobDoor*>(ent);

			if (open)
			{
				if (door->IsAtClosedPosition())
				{
					// Other door is at closed position, let the handle tap
					door->Open(false);
				}
				else
				{
					// The other door is open or in an intermediate state, just call open
					door->OpenDoor(false);
				}
			}
			else
			{
				door->Close(false);
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), openPeer.c_str());
		}
	}
}

void CFrobDoor::LockPeers()
{
	LockUnlockPeers(true);
}

void CFrobDoor::UnlockPeers()
{
	LockUnlockPeers(false);
}

void CFrobDoor::LockUnlockPeers(const bool lock)
{
	// Go through the list and issue the call
	for (int i = 0; i < m_LockPeers.Num(); i++)
	{
		const idStr& lockPeer = m_LockPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", lockPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(lockPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", lockPeer.c_str());

			CFrobDoor* door = static_cast<CFrobDoor*>(ent);
			
			if (lock)
			{
				door->Lock(false);
			}
			else
			{
				door->Unlock(false);
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found or of wrong type\r", lockPeer.c_str());
		}
	}
}

void CFrobDoor::TapPeers()
{
	// In master mode, tap the handles of the master_open chain too
	// Cycle through our "open peers" list and issue the call to them
	for (int i = 0; i < m_OpenPeers.Num(); i++)
	{
		const idStr& openPeer = m_OpenPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", openPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(openPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for tapping\r", openPeer.c_str());
			CFrobDoor* other = static_cast<CFrobDoor*>(ent);

			if (other->GetDoorhandle() != NULL)
			{
				other->GetDoorhandle()->Tap();
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), openPeer.c_str());
		}
	}
}

void CFrobDoor::AddOpenPeer(const idStr& peerName)
{
	// Find the entity and check if it's actually a door
	idEntity* entity = gameLocal.FindEntity(peerName);

	if (entity != NULL && entity->IsType(CFrobDoor::Type))
	{
		m_OpenPeers.AddUnique(peerName);
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: open_peer %s added to local list (%u)\r", name.c_str(), peerName.c_str(), m_OpenPeers.Num());
	}
	else
	{
		DM_LOG(LC_ENTITY, LT_ERROR)LOGSTRING("open_peer [%s] not spawned or of wrong type.\r", peerName.c_str());
	}
}

void CFrobDoor::RemoveOpenPeer(const idStr& peerName)
{
	m_OpenPeers.Remove(peerName);
}

void CFrobDoor::AddLockPeer(const idStr& peerName)
{
	// Find the entity and check if it's actually a door
	idEntity* entity = gameLocal.FindEntity(peerName);

	if (entity != NULL && entity->IsType(CFrobDoor::Type))
	{
		m_LockPeers.AddUnique(peerName);
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: lock_peer %s added to local list (%u)\r", name.c_str(), peerName.c_str(), m_LockPeers.Num());
	}
	else
	{
		DM_LOG(LC_ENTITY, LT_ERROR)LOGSTRING("lock_peer [%s] not spawned or of wrong type.\r", peerName.c_str());
	}
}

void CFrobDoor::RemoveLockPeer(const idStr& peerName)
{
	m_LockPeers.Remove(peerName);
}

CFrobDoorHandle* CFrobDoor::GetDoorhandle()
{
	return (m_Doorhandles.Num() > 0) ? m_Doorhandles[0].GetEntity() : NULL;
}

void CFrobDoor::AddDoorhandle(CFrobDoorHandle* handle)
{
	// Store the pointer and the original position
	idEntityPtr<CFrobDoorHandle> handlePtr;
	handlePtr = handle;

	if (m_Doorhandles.FindIndex(handlePtr) != -1)
	{
		return; // handle is already known
	}

	m_Doorhandles.Append(handlePtr);

	// Let the handle know about us
	handle->SetDoor(this);

	// Set up the frob peer relationship between the door and the handle
	m_FrobPeers.AddUnique(handle->name);
	handle->AddFrobPeer(name);
	handle->SetFrobable(m_bFrobable);
}

void CFrobDoor::AutoSetupDoorHandles()
{
	// Find a suitable teamchain member
	idEntity* part = FindMatchingTeamEntity(CFrobDoorHandle::Type);

	while (part != NULL)
	{
		// Found the handle, set it up
		AddDoorhandle(static_cast<CFrobDoorHandle*>(part));

		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: Auto-added door handle %s to local list.\r", name.c_str(), part->name.c_str());

		// Get the next handle
		part = FindMatchingTeamEntity(CFrobDoorHandle::Type, part);
	}

	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
		if (handle == NULL) continue;

		// The first handle is the master, all others get their master flag set to FALSE
		handle->SetMasterHandle(i == 0);
	}
}

void CFrobDoor::AutoSetupDoubleDoor()
{
	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();

	if (doubleDoor != NULL)
	{
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: Auto-setting up %s as double door.\r", name.c_str(), doubleDoor->name.c_str());

		if (spawnArgs.GetBool("auto_setup_double_door_frob_peer", "0"))
		{
			// Add the door to our frob_peer set
			m_FrobPeers.AddUnique(doubleDoor->name);

			// Add ourselves to the double door as frob peer
			doubleDoor->AddFrobPeer(name);
			doubleDoor->SetFrobable(m_bFrobable);
		}

		if (spawnArgs.GetBool("auto_setup_double_door_open_peer", "0"))
		{
			// Add "self" to the peers of the other door
			doubleDoor->AddOpenPeer(name);
			// Now add the name of the other door to our own peer list
			AddOpenPeer(doubleDoor->name);
		}

		if (spawnArgs.GetBool("auto_setup_double_door_lock_peer", "0"))
		{
			// Add "self" to the peers of the other door
			doubleDoor->AddLockPeer(name);
			// Now add the name of the other door to our own peer list
			AddLockPeer(doubleDoor->name);
		}
	}
}

bool CFrobDoor::PreLock(bool bMaster)
{
	if (!CBinaryFrobMover::PreLock(bMaster))
	{
		return false;
	}

	// Allow closing if all lock peers are at their closed position
	return AllLockPeersAtClosedPosition();
}

bool CFrobDoor::AllLockPeersAtClosedPosition()
{
	// Go through the list and check whether the lock peers are closed
	for (int i = 0; i < m_LockPeers.Num(); i++)
	{
		const idStr& lockPeer = m_LockPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", lockPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(lockPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", lockPeer.c_str());

			CFrobDoor* door = static_cast<CFrobDoor*>(ent);
			
			if (!door->IsAtClosedPosition())
			{
				return false;
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found or of wrong type\r", lockPeer.c_str());
		}
	}

	return true;
}

CFrobDoorHandle* CFrobDoor::GetNearestHandle(const idVec3& pos) const
{
	// Skip calculation if only one doorhandle present in the first place
	if (m_Doorhandles.Num() == 1) 
	{
		return m_Doorhandles[0].GetEntity();
	}

	float bestDistanceSqr = idMath::INFINITY;
	CFrobDoorHandle* returnValue = NULL;

	for (int i = 0; i < m_Doorhandles.Num(); ++i)
	{
		CFrobDoorHandle* candidate = m_Doorhandles[i].GetEntity();

		if (candidate == NULL) continue;

		float candidateDistanceSqr = (candidate->GetPhysics()->GetOrigin() - pos).LengthSqr();

		if (candidateDistanceSqr < bestDistanceSqr)
		{
			// Found a new nearest handle
			returnValue = candidate;
			bestDistanceSqr = candidateDistanceSqr;
		}
	}

	return returnValue;
}

bool CFrobDoor::GetPhysicsToSoundTransform(idVec3 &origin, idMat3 &axis)
{
	// This will kick in for doors without any handles, these are playing their
	// sounds from the nearest point to the player's eyes, mid-bounding-box.
	const idBounds& bounds = GetPhysics()->GetAbsBounds();

	// grayman #4337 - Using the bounding box might place the "nearest point"
	// inside the wall surrounding the door. Use a bounding box that's smaller
	// than the door's. This runs the risk of setting the nearest point on the
	// wrong side of the visportal, but most doors are 4 units thick, and the door's
	// bounding box is 1 larger than the door, so most bounding boxes will be
	// 6 units thick. If most portals are down the middle of the door, shrinking
	// the bounding box by 1.5 units seems safe.

	const idBounds& smallerBounds = bounds.Expand(-1.5f);

	const idPlayer* player = gameLocal.GetLocalPlayer();
	idVec3 eyePos = player ? player->GetEyePosition() : vec3_zero; // #4075 Don't try accessing player's eye pos before he spawns

	// greebo: Choose the corner which is nearest to the player's eye position
	origin.x = (idMath::Fabs(smallerBounds[0].x - eyePos.x) < idMath::Fabs(smallerBounds[1].x - eyePos.x)) ? smallerBounds[0].x : smallerBounds[1].x;
	origin.y = (idMath::Fabs(smallerBounds[0].y - eyePos.y) < idMath::Fabs(smallerBounds[1].y - eyePos.y)) ? smallerBounds[0].y : smallerBounds[1].y;
	origin.z = (smallerBounds[0].z + smallerBounds[1].z) * 0.5f;

	axis.Identity();

	if (areaPortal) {
		//stgatilov #5462: ensure origin and player are on the same side of the door portal
		idPlane portalPlane = gameRenderWorld->GetPortalPlane(areaPortal);
		assert(fabs(portalPlane.Normal().Length() - 1.0f) <= 1e-3f);

		float eyeDist = portalPlane.Distance(eyePos);
		float originDist = portalPlane.Distance(origin);
		if (eyeDist * originDist < 0.0f) {	//different sides
			//move origin to the portal plane plus one unit
			float shift = fabs(originDist) + 1.0f;
			//don't move by more than 30% of door diameter
			float cap = 0.3f * bounds.GetSize().Length();
			if (shift > cap)
				shift = cap;
			idVec3 displacement = (originDist < 0.0f ? 1.0f : -1.0f) * shift * portalPlane.Normal();
			origin += displacement;
		}
	}

	// The caller expects the origin in local space
	origin -= GetPhysics()->GetOrigin();

	//gameRenderWorld->DebugArrow(colorWhite, GetPhysics()->GetOrigin() + origin, eyePos, 0, 5000);

	return true;
}

int CFrobDoor::FrobMoverStartSound(const char* soundName)
{
	if (m_Doorhandles.Num() > 0)
	{
		// greebo: Find the handle nearest to the player, as one of the doorhandles could be 
		// behind a closed visportal.
		CFrobDoorHandle* handle = GetNearestHandle(gameLocal.GetLocalPlayer()->GetEyePosition());

		if (handle != NULL)
		{
			// Let the sound play from the handle, but use the soundshader
			// as defined on this entity.
			idStr sound = spawnArgs.GetString(soundName, "");

			if (sound.IsEmpty())
			{
				gameLocal.Warning("Cannot find sound %s on door %s", soundName, name.c_str());
				return 0;
			}

			const idSoundShader* shader = declManager->FindSound(sound);

			int length = 0;
			handle->StartSoundShader(shader, SND_CHANNEL_ANY, 0, false, &length);

			//gameRenderWorld->DebugArrow(colorWhite, handle->GetPhysics()->GetOrigin(), gameLocal.GetLocalPlayer()->GetEyePosition(), 1, 5000);

			return length;
		}
	}

	// No handle or NULL handle, just call the base class
	return CBinaryFrobMover::FrobMoverStartSound(soundName);
}

void CFrobDoor::Event_GetDoorhandle()
{
	idThread::ReturnEntity(m_Doorhandles.Num() > 0 ? m_Doorhandles[0].GetEntity() : NULL);
}

void CFrobDoor::Event_OpenDoor(float master)
{
	OpenDoor(master != 0.0f ? true : false);
}

void CFrobDoor::Event_HandleLockRequest()
{
	// Check if all the peers are at their "closed" position, if yes: lock the door(s), if no: postpone the event
	if (AllLockPeersAtClosedPosition())
	{
		// Yes, all lock peers are at their "closed" position, lock ourselves and all peers
		Lock(true);
	}
	else
	{
		// One or more peers are not at their closed position (yet), postpone the event
		PostEventMS(&EV_TDM_FrobMover_HandleLockRequest, LOCK_REQUEST_DELAY);
	}
}

// grayman #2859

void CFrobDoor::SetLastUsedBy(idEntity* ent)
{
	m_lastUsedBy = ent;
}

idEntity* CFrobDoor::GetLastUsedBy() const
{
	return m_lastUsedBy.GetEntity();
}

// grayman #1327

void CFrobDoor::SetSearching(idEntity* ent)
{
	m_searching = ent;
}

idEntity* CFrobDoor::GetSearching() const
{
	return m_searching.GetEntity();
}

// grayman #3104

void CFrobDoor::SetWasFoundLocked(bool state)
{
	m_wasFoundLocked = state;
}

bool CFrobDoor::GetWasFoundLocked() const
{
	return m_wasFoundLocked;
}

// grayman #2866 - GetDoorHandlingEntities() finds the door handling entities when a door uses them.

bool CFrobDoor::GetDoorHandlingEntities( idAI* owner, idList< idEntityPtr<idEntity> > &list )
{
	idEntity* frontEnt = NULL;
	idEntity* backEnt = NULL;
	bool positionsFound = false;
	list.Clear();

	idVec3 frobDoorOrg = GetPhysics()->GetOrigin();
	idVec3 ownerOrg = owner->GetPhysics()->GetOrigin();
	idVec3 gravity = gameLocal.GetGravity();
	const idVec3& closedPos = frobDoorOrg + GetClosedPos();

	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	idVec3 ownerDir = ownerOrg - frobDoorOrg;
	ownerDir.z = 0;
	idVec3 doorNormal = dir.Cross(gravity);
	float ownerTest = doorNormal * ownerDir;

	// check for custom door handling positions
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("door_handle_position"); kv != NULL; kv = spawnArgs.MatchPrefix("door_handle_position", kv))
	{
		idStr posStr = kv->GetValue();
		idEntity* doorHandlingPosition = gameLocal.FindEntity(posStr);

		if ( doorHandlingPosition )
		{
			idVec3 posOrg = doorHandlingPosition->GetPhysics()->GetOrigin();
			idVec3 posDir = posOrg - frobDoorOrg;
			posDir.z = 0;
			float posTest = doorNormal * posDir;

			if (posTest * ownerTest > 0)
			{
				frontEnt = doorHandlingPosition; // door handling position in front of the door
			}
			else
			{
				backEnt = doorHandlingPosition; // door handling position on the far side of the door
			}
			positionsFound = true;
		}
	}

	if ( positionsFound )
	{
		idEntityPtr<idEntity> &entityPtr1 = list.Alloc();
		entityPtr1 = frontEnt;
		idEntityPtr<idEntity> &entityPtr2 = list.Alloc();
		entityPtr2 = backEnt;
	}

	return positionsFound;
}

// grayman #3643 - Returns a two-element list with the front controller
// as the first element and the back controller as the second.
/*
bool CFrobDoor::GetDoorControllers( idAI* owner, idList< idEntityPtr<idEntity> > &list )
{
	idEntity* frontEnt = NULL;
	idEntity* backEnt = NULL;
	bool controllersFound = false;
	list.Clear();

	idVec3 frobDoorOrg = GetPhysics()->GetOrigin();
	idVec3 ownerOrg = owner->GetPhysics()->GetOrigin();
	idVec3 gravity = gameLocal.GetGravity();
	const idVec3& closedPos = frobDoorOrg + GetClosedPos();

	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	idVec3 ownerDir = ownerOrg - frobDoorOrg;
	ownerDir.z = 0;
	idVec3 doorNormal = dir.Cross(gravity);
	float ownerTest = doorNormal * ownerDir;

	// check for controllers
	for ( int i = 0 ; i < m_controllers.Num() ; i++ )
	{
		idEntity* e = m_controllers[i].GetEntity();
		if (e)
		{
			idVec3 org = e->GetPhysics()->GetOrigin();
			idVec3 dir = org - frobDoorOrg;
			org.z = 0;
			float test = doorNormal * dir;

			if (test * ownerTest > 0)
			{
				frontEnt = e; // door handling controller on front side of the door
			}
			else
			{
				backEnt = e; // door handling controller on back side of the door
			}
			controllersFound = true;
		}
	}

	if ( controllersFound )
	{
		idEntityPtr<idEntity> &entityPtr1 = list.Alloc();
		entityPtr1 = frontEnt;
		idEntityPtr<idEntity> &entityPtr2 = list.Alloc();
		entityPtr2 = backEnt;
	}

	return controllersFound;
}
*/
// grayman #3042 - need to think while moving

void CFrobDoor::Think( void )
{
	idEntity::Think();

	if ( IsMoving() )
	{
		UpdateSoundLoss();
	}
}

// grayman #3643 - keep a list of controllers for this door
void CFrobDoor::AddController(idEntity* newController)
{
	idEntityPtr<idEntity> controllerPtr;
	controllerPtr = newController;
	if ( m_controllers.FindIndex( controllerPtr ) < 0 ) // don't add if already there
	{
		m_controllers.Append(controllerPtr);
	}
}

// grayman #3643 - keep a list of door handling positions for this door
void CFrobDoor::AddDHPosition(idEntity* newDHPosition)
{
	idEntityPtr<idEntity> dhpPtr;
	dhpPtr = newDHPosition;
	m_doorHandlingPositions.Append(dhpPtr);
}

// grayman #3643 - find the mid position for door handling

void CFrobDoor::GetMidPos(float rotationAngle)
{
	idVec3 midPos;

	if (rotationAngle != 0)
	{
		// rotating door
		const idVec3& frobDoorOrg = GetPhysics()->GetOrigin();
		const idVec3& closedPos = frobDoorOrg + GetClosedPos();

		idVec3 dir = closedPos - frobDoorOrg;
		dir.z = 0;
		float doorWidth = dir.LengthFast();
		idVec3 dirNorm = dir;
		dirNorm.NormalizeFast();

		idVec3 openDirNorm = m_OpenDir;
		openDirNorm.z = 0;
		openDirNorm.NormalizeFast();

		idVec3 parallelMidOffset = dirNorm;
		parallelMidOffset *= doorWidth/2; // grayman #2712 - align with center of closed door position

		idVec3 normalMidOffset = openDirNorm;
		idBounds frobDoorBounds = GetPhysics()->GetAbsBounds();
		float size = AI_SIZE/2;

		// front
		idVec3 frontMidOffset = normalMidOffset*1.25*doorWidth; // grayman #2712 - when the door swings away from you, clear it before ending the task
		midPos = closedPos - parallelMidOffset + frontMidOffset;
		midPos.z = frobDoorBounds[0].z + 1;
		m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_MID] = midPos;

		// back
		idVec3 backMidOffset = -size * 3.0 * normalMidOffset; // don't have to go so far when the door swings toward you
		midPos = closedPos - parallelMidOffset + backMidOffset;
		midPos.z = frobDoorBounds[0].z + 1;
		m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_MID] = midPos;
	}
	else // sliding door
	{
		idVec3 openDir = GetTranslation();
		idBox doorBox = GetClosedBox();
		idVec3 center = doorBox.GetCenter();
		idVec3 extents = doorBox.GetExtents();
		idVec3 min = center - extents;
		idVec3 max = center + extents;
		if ( extents.x > extents.y )
		{
			max.y = min.y;
		}
		else
		{
			max.x = min.x;
		}
		idVec3 doorFace = max - min;
		idVec3 normal = doorFace.Cross(openDir);
		normal.NormalizeFast();

		// front
		
		midPos = center + 40*normal;
		midPos.z = min.z + 1;
		m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_MID] = midPos;
		
		// back
		
		midPos = center - 40*normal;
		midPos.z = min.z + 1;
		m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_MID] = midPos;
	}
}

// grayman #3755 - Find the side markers for door handling.
// This has to follow a call to GetMidPos().

void CFrobDoor::GetSideMarkers()
{
	idVec3 center = GetClosedBox().GetCenter();
	idVec3 midFront = m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_MID];
	center.z = midFront.z;
	idVec3 center2midFront = midFront - center;
	center2midFront.Normalize();
	m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_SIDEMARKER] = center - 100*center2midFront;
	m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_SIDEMARKER]  = center + 100*center2midFront;
}

// grayman #3643 - register door opening data for each side of the door
void CFrobDoor::GetDoorHandlingPositions()
{
	// No need to establish door handling positions if the door can't
	// be used by AI.

	// Can't pass through doors that don't rotate on the z-axis.

	idVec3 rotationAxis = GetRotationAxis();
	if ((rotationAxis.z == 0) && ((rotationAxis.x != 0) || (rotationAxis.y != 0)))
	{
		return;
	}

	// Since door handling positions are only for the use of AI, if there
	// are no AI in the map (yet), there's no AAS file, so trying to set up
	// door handling positions will fail. To let mappers create maps with no
	// AI in them, test for AAS, and skip this method if none is available.

	idAAS *aas = gameLocal.GetAAS("aas32");
	if (aas == NULL)
	{
		return;
	}

	// There are 2 conditions that dictate how the positions are determined:
	//
	// 1 - the door rotates or slides
	// 2 - the door is activated by frobbing, and/or controllers (switches/buttons), and/or door handling positions

	// A door has two sides. We keep data per side. For a rotating door,
	// side 0 is the direction the door moves, and side 1 is the other side.

	// determine which is side 0

	idAngles rotate = spawnArgs.GetAngles("rotate", "0 90 0");
	m_rotates = ( (rotate.yaw != 0) || (rotate.pitch != 0) || (rotate.roll != 0) );

	idEntity* frontPosEnt = NULL;
	idEntity* backPosEnt = NULL;

	idEntity* frontController = NULL;
	idEntity* backController = NULL;

	for ( int i = 0 ; i < DOOR_SIDES ; i++ )
	{
		for ( int j = 0 ; j < NUM_DOOR_POSITIONS ; j++ )
		{
			m_doorPositions[i][j].Zero();
		}
	}

	// Default: in case door is controlled by frobbing

	if (m_rotates)
	{
		GetForwardPos();
		GetBehindPos();
		GetMidPos(rotate.yaw);
		GetSideMarkers();
	}
	else // slides
	{
		GetMidPos(0);
		GetSideMarkers();
		m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_FRONT] = m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_MID];
		m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_BACK]   = m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_MID];
		m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_BACK]  = m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_MID];
		m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_FRONT]  = m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_MID];
	}

	// If door handling positions exist, they override the default frobbing positions

	if (m_doorHandlingPositions.Num() > 0)
	{
		for ( int i = 0 ; i < m_doorHandlingPositions.Num() ; i++ ) // for doors that use door handling positions
		{
			// determine which side of the door the entity sits on
			idEntity* e = m_doorHandlingPositions[i].GetEntity();
			if (e)
			{
				bool isInFront = false;
				if (m_rotates)
				{
					idVec3 v = e->GetPhysics()->GetOrigin() - GetClosedBox().GetCenter();
					float dot = v * m_OpenDir;
					if (dot > 0)
					{
						isInFront = true;
					}
				}
				else // sliding door
				{
					float dist2frontfront = (e->GetPhysics()->GetOrigin() - m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_FRONT]).LengthFast();
					float dist2backfront  = (e->GetPhysics()->GetOrigin() - m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_FRONT]).LengthFast();
					if (dist2frontfront < dist2backfront)
					{
						isInFront = true;
					}
				}

				if (isInFront)
				{
					if (frontPosEnt == NULL)
					{
						frontPosEnt = e;
					}
				}
				else if (backPosEnt == NULL)
				{
					backPosEnt = e;
				}
			}

			if (frontPosEnt && backPosEnt)
			{
				break;
			}
		}

		idVec3 goal;
		if ( frontPosEnt != NULL )
		{
			goal = frontPosEnt->GetPhysics()->GetOrigin();
			m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_FRONT] = goal;
			m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_BACK]   = goal;
		}

		if ( backPosEnt != NULL )
		{
			goal = backPosEnt->GetPhysics()->GetOrigin();
			m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_BACK] = goal;
			m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_FRONT] = goal;
		}
		// Clear the door handle positions list and store our found
		// entities in slots 0 and 1. It's possible
		// that one or the other might be NULL (doesn't exist),
		// but not both.

		m_doorHandlingPositions.Clear();
		idEntityPtr<idEntity> ptr0;
		idEntityPtr<idEntity> ptr1;
		ptr0 = frontPosEnt;
		ptr1 = backPosEnt;
		m_doorHandlingPositions.Append(ptr0); // side 0 entity
		m_doorHandlingPositions.Append(ptr1); // side 1 entity
	}
	
	// If door controllers exist, they override the default frobbing and door handling positions

	if (m_controllers.Num() > 0) // Is the door controlled by buttons/switches/levers?
	{
		idVec3 doorCenter = GetClosedBox().GetCenter(); // grayman #3643
		for ( int i = 0 ; i < m_controllers.Num() ; i++)
		{
			// determine which side of the door the entity sits on
			idEntity* e = m_controllers[i].GetEntity();
			if (e)
			{
				bool isInFront = false;
				if (m_rotates)
				{
					idVec3 v = e->GetPhysics()->GetOrigin() - doorCenter;
					float dot = v * m_OpenDir;
					if (dot > 0)
					{
						isInFront = true;
					}
				}
				else // sliding door
				{
					float dist2frontfront = (e->GetPhysics()->GetOrigin() - m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_FRONT]).LengthFast();
					float dist2backfront  = (e->GetPhysics()->GetOrigin() - m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_FRONT]).LengthFast();
					if (dist2frontfront < dist2backfront)
					{
						isInFront = true;
					}
				}

				if (isInFront)
				{
					if (frontController == NULL)
					{
						frontController = e;
					}
				}
				else if (backController == NULL)
				{
					backController = e;
				}
			}

			if (frontController && backController)
			{
				break;
			}
		}

		float standOff; // not used
		idVec3 goal;

		if ( frontController != NULL )
		{
			static_cast<CBinaryFrobMover*>(frontController)->GetSwitchGoal(goal,standOff,0);
			m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_FRONT] = goal;
			m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_BACK]   = goal;
		}

		if ( backController != NULL )
		{
			static_cast<CBinaryFrobMover*>(backController)->GetSwitchGoal(goal,standOff,0);
			m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_BACK] = goal;
			m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_FRONT] = goal;
		}

		// Clear the controller list and store our found
		// controllers in slots 0 and 1. It's possible
		// that one or the other might be NULL (doesn't exist),
		// but not both.

		m_controllers.Clear();
		idEntityPtr<idEntity> ptr0;
		idEntityPtr<idEntity> ptr1;
		ptr0 = frontController;
		ptr1 = backController;
		m_controllers.Append(ptr0); // side 0 controller
		m_controllers.Append(ptr1); // side 1 controller
	}
}

// grayman #3643 - xfer the door's locking situation to the controllers
void CFrobDoor::SetControllerLocks()
{
	int num = m_controllers.Num();
	if (num > 0)
	{
		for ( int i = 0 ; i < num ; i++ )
		{
			idEntity* e = m_controllers[i].GetEntity();
			if (e && e->IsType(CBinaryFrobMover::Type))
			{
				PickableLock* controllerLock = static_cast<CBinaryFrobMover*>(e)->GetLock();
				// Read the door's spawnargs to get the lock settings
				controllerLock->InitFromSpawnargs(spawnArgs);

				// Set the controller's lockpick type from the door's
				idStr pick = spawnArgs.GetString("lock_picktype");
				e->spawnArgs.Set("lock_picktype",pick.c_str());

				// grayman #4262 - Set the controller's failed lock sound from the door's
				idStr wrongKey = spawnArgs.GetString("snd_wrong_key");
				e->spawnArgs.Set("snd_wrong_key",wrongKey.c_str());

				// Set the controller's used_by list from the door's
				for (const idKeyValue* kv = spawnArgs.MatchPrefix("used_by"); kv != NULL; kv = spawnArgs.MatchPrefix("used_by", kv))
				{
					// argh, backwards compatibility, must keep old used_by format for used_by_name
					// explicitly ignore stuff with other prefixes
					idStr kn = kv->GetKey();
					if ( !kn.IcmpPrefix("used_by_inv_name") || !kn.IcmpPrefix("used_by_category") || !kn.IcmpPrefix("used_by_classname") )
					{
						continue;
					}

					// Add each entity name to the controller's list
					e->m_UsedByName.AddUnique(kv->GetValue());
				}
			}
		}

		Event_SetKey("open_on_unlock", "0"); // tell door not to open on unlock
		Unlock(true);
	}
}

void CFrobDoor::GetForwardPos()
{
	const idVec3& frobDoorOrg = GetPhysics()->GetOrigin();
	const idVec3& closedPos = frobDoorOrg + GetClosedPos();

	idBounds frobDoorBounds = GetPhysics()->GetAbsBounds();

	float size = AI_SIZE/2;

	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	idVec3 dirNorm = dir;
	dirNorm.NormalizeFast();

	idVec3 openDirNorm = m_OpenDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	idVec3 parallelAwayOffset = dirNorm * size * 1.4f;
	idVec3 normalAwayOffset = openDirNorm * size * 2.5;

	idVec3 awayPos = closedPos - parallelAwayOffset - normalAwayOffset;
	awayPos.z = frobDoorBounds[0].z + 5;
	m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_BACK] = awayPos;
	m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_FRONT] = awayPos;
}

void CFrobDoor::GetBehindPos()
{
	// calculate where to stand when the door swings towards us
	const idVec3& frobDoorOrg = GetPhysics()->GetOrigin();
	const idVec3& openDir = GetOpenDir();
	const idVec3& closedPos = frobDoorOrg + GetClosedPos();

	idBounds frobDoorBounds = GetPhysics()->GetAbsBounds();

	idBounds bounds(idVec3( -16, -16, 0 ),idVec3( 16, 16, 68 )); // idActor bounds
	float size = bounds[1][0];
	idTraceModel trm(bounds);
	idClipModel clip(trm);

	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	idVec3 dirNorm = dir;
	dirNorm.NormalizeFast();
	float dist = dir.LengthFast();
	
	idVec3 openDirNorm = openDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	// next to the door
	idVec3 parallelTowardOffset = dirNorm;
	parallelTowardOffset *= dist + size * 2;

	idVec3 normalTowardOffset = openDirNorm;
	normalTowardOffset *= size * 2;

	idVec3 towardPos = frobDoorOrg + parallelTowardOffset + normalTowardOffset;
	towardPos.z = frobDoorBounds[0].z + 5;

	// GetBehindPos() spends time checking fit because the AI stands
	// to one side on this side of the door, and furniture, etc. might
	// get in the way. GetForwardPos() doesn't have that problem.

	// check if we can stand at this position
	int contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, NULL);

	idAAS *aas = gameLocal.GetAAS("aas32");

	int areaNum = aas->PointReachableAreaNum(towardPos, bounds, AREA_REACHABLE_WALK); // grayman #3788
	//int areaNum = aas->PointAreaNum(towardPos);

	if ( contents || (areaNum == 0) || (GetOpenPeersNum() > 0) )
	{
		// this position is either blocked, in the void or can't be used since the door has open peers
		// try at 45° swinging angle
		parallelTowardOffset = dirNorm;

		normalTowardOffset = openDirNorm;

		towardPos = parallelTowardOffset + normalTowardOffset;
		towardPos.NormalizeFast();
		towardPos *= (dist + size * 2);
		towardPos += frobDoorOrg;
		towardPos.z = frobDoorBounds[0].z + 5;

		contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, NULL);

		areaNum = aas->PointReachableAreaNum(towardPos, bounds, AREA_REACHABLE_WALK); // grayman #3788
		//areaNum = aas->PointAreaNum(towardPos);

		if ( contents || (areaNum == 0) || (GetOpenPeersNum() > 0) )
		{
			// not useable, try in front of the door far enough away
			parallelTowardOffset = dirNorm * (dist/2.0f); // grayman #3786
			//parallelTowardOffset = dirNorm * size * 1.2f;

			normalTowardOffset = openDirNorm;
			normalTowardOffset *= dist + 2.5f * size;

			towardPos = frobDoorOrg + parallelTowardOffset + normalTowardOffset;
			towardPos.z = frobDoorBounds[0].z + 5;

			contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, NULL);

			areaNum = aas->PointReachableAreaNum(towardPos, bounds, AREA_REACHABLE_WALK); // grayman #3788
			//areaNum = aas->PointAreaNum(towardPos);

			if ( contents || (areaNum == 0) )
			{
				// TODO: no suitable position found
			}
		}
	}
	m_doorPositions[DOOR_SIDE_FRONT][DOOR_POS_FRONT] = towardPos;
	m_doorPositions[DOOR_SIDE_BACK][DOOR_POS_BACK]   = towardPos;
}

// grayman #3643 - retrieve a particular door controller
idEntityPtr<idEntity> CFrobDoor::GetDoorController(int side)
{
	idEntityPtr<idEntity> ePtr;
	ePtr = NULL;
	int num = m_controllers.Num();

	switch (side)
	{
	case DOOR_SIDE_FRONT:
		if (num >= 1)
		{
			ePtr = m_controllers[DOOR_SIDE_FRONT];
		}
		break;
	case DOOR_SIDE_BACK:
		if (num == 2)
		{
			ePtr = m_controllers[DOOR_SIDE_BACK];
		}
		break;
	default:
		break;
	}

	return ePtr;
}

// grayman #4882 - retrieve a door peek entity
idEntityPtr<idPeek> CFrobDoor::GetDoorPeekEntity()
{
	return m_peek;
}

// grayman #3643 - retrieve a particular door handle position
idEntityPtr<idEntity> CFrobDoor::GetDoorHandlePosition(int side)
{
	idEntityPtr<idEntity> ePtr;
	ePtr = NULL;
	int num = m_doorHandlingPositions.Num();

	switch (side)
	{
	case DOOR_SIDE_FRONT:
		if (num >= 1)
		{
			ePtr = m_doorHandlingPositions[DOOR_SIDE_FRONT];
		}
		break;
	case DOOR_SIDE_BACK:
		if (num == 2)
		{
			ePtr = m_doorHandlingPositions[DOOR_SIDE_BACK];
		}
		break;
	default:
		break;
	}

	return ePtr;
}

void CFrobDoor::LockControllers(bool bMaster) // grayman #3643
{
	for ( int i = 0 ; i < m_controllers.Num() ; i++ )
	{
		idEntity* e = m_controllers[i].GetEntity();
		if (e)
		{
			static_cast<CBinaryFrobMover*>(e)->Lock(bMaster);
		}
	}
}

bool CFrobDoor::UnlockControllers(bool bMaster) // grayman #3643
{
	// On the first unlock, capture whether that unlock is going
	// to do an automatic open of the door. On a subsequent
	// unlock of the other controller, temporarily turn off the
	// spawnarg that tells it to automatically open the door, since
	// we've already done so.

	bool result = true;
	bool capturedResult = false;
	for ( int i = 0 ; i < m_controllers.Num() ; i++ )
	{
		idEntity* e = m_controllers[i].GetEntity();
		if (e)
		{
			if (!capturedResult)
			{
				result = static_cast<CBinaryFrobMover*>(e)->Unlock(bMaster);
				capturedResult = true;
			}
			else
			{
				bool openOnUnlock = e->spawnArgs.GetBool("open_on_unlock");
				e->Event_SetKey("open_on_unlock", "0"); // tell controller not to open on unlock
				static_cast<CBinaryFrobMover*>(e)->Unlock(bMaster);
				if (openOnUnlock)
				{
					e->Event_SetKey("open_on_unlock", "1"); // reset spawnarg
				}
			}
		}
	}
	return result;
}

// grayman #3643
bool CFrobDoor::IsLocked()
{
	bool isLocked = false;

	if (m_controllers.Num() > 0 )
	{
		for ( int i = 0 ; i < m_controllers.Num() ; i++ )
		{
			idEntity* e = m_controllers[i].GetEntity();
			if (e && e->IsType(CBinaryFrobMover::Type))
			{
				CBinaryFrobMover* controller = static_cast<CBinaryFrobMover*>(e);
				if (controller->IsLocked())
				{
					isLocked = true;
					break;
				}
			}
		}
	}
	else // no controllers, so check the door
	{
		isLocked = m_Lock->IsLocked();
	}

	return isLocked;
}

// grayman #3748
void CFrobDoor::PushDoorHard()
{
	if (!m_AIPushingDoor)
	{
		m_AIPushingDoor = true;
		m_previouslyFrobable = m_bFrobable; // save so you can restore it later
		SetFrobable(false); // don't set m_bFrobable directly
		m_previouslyPushingPlayer = SetCanPushPlayer(true); // returns previous value for later restore
		prevTransSpeed = GetTransSpeed();
		if (prevTransSpeed > 0) // grayman #3755
		{
			// use trans speed
			SetTransSpeed(m_speedFactor*prevTransSpeed); // increase door speed
		}
		else
		{
			// use move time
			prevMoveTime = GetMoveTime();
			Event_SetMoveTime(((float)prevMoveTime)/(m_speedFactor*1000.0f)); // cut move time
		}
	}
}

// grayman #3748
void CFrobDoor::StopPushingDoorHard()
{
	if (m_AIPushingDoor)
	{
		m_AIPushingDoor = false;
		SetCanPushPlayer(m_previouslyPushingPlayer);
		SetFrobable(m_previouslyFrobable); // don't set m_bFrobable directly
		if (prevTransSpeed > 0) // grayman #3755
		{
			// reset previous move speed
			SetTransSpeed(prevTransSpeed);
		}
		else
		{
			// reset door move time 
			Event_SetMoveTime(((float)prevMoveTime)/1000.0f);
		}
	}
}

// grayman #3755
bool CFrobDoor::IsPushingHard()
{
	return m_AIPushingDoor;
}






