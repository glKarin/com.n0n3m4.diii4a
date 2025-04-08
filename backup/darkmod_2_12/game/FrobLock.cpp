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

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "FrobLock.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/Category.h"

const idEventDef EV_TDM_FrobLock_TriggerTargets("_EV_TDM_FrobLock_TriggerTargets", EventArgs(), EV_RETURNS_VOID, "internal"); // triggers general targets
const idEventDef EV_TDM_FrobLock_TriggerLockTargets("_EV_TDM_FrobLock_TriggerLockTargets", EventArgs(), EV_RETURNS_VOID, "internal"); // triggers lock targets
const idEventDef EV_TDM_FrobLock_TriggerUnlockTargets("_EV_TDM_FrobLock_TriggerUnlockTargets", EventArgs(), EV_RETURNS_VOID, "internal"); // triggers unlock targets
const idEventDef EV_TDM_FrobLock_ClearPlayerImmobilization("_EV_TDM_FrobLock_ClearPlayerImmobilization", 
	EventArgs('e', "", ""), EV_RETURNS_VOID, "internal"); // allows player to handle weapons again

const idEventDef EV_TDM_FrobLock_Open("Open", EventArgs(), EV_RETURNS_VOID, ""); // attempts to open the lock
const idEventDef EV_TDM_FrobLock_Lock("Lock", EventArgs(), EV_RETURNS_VOID, "Locks the froblock.");
const idEventDef EV_TDM_FrobLock_Unlock("Unlock", EventArgs(), EV_RETURNS_VOID, "Unlocks the froblock.");
const idEventDef EV_TDM_FrobLock_ToggleLock("ToggleLock", EventArgs(), EV_RETURNS_VOID,
	"Toggles the lock state. Unlocked froblocks will be locked and vice versa.\n" \
	"The notes above concerning Unlock() still apply if this call unlocks the froblock. ");
const idEventDef EV_TDM_FrobLock_IsLocked("IsLocked", EventArgs(), 'f', "Returns true (nonzero) if the froblock is currently locked.");
const idEventDef EV_TDM_FrobLock_IsPickable("IsPickable", EventArgs(), 'f', "Returns true (nonzero) if this froblock is pickable.");

CLASS_DECLARATION( idStaticEntity, CFrobLock )
	EVENT( EV_PostSpawn,							CFrobLock::PostSpawn )
	EVENT( EV_TDM_Lock_StatusUpdate,				CFrobLock::Event_Lock_StatusUpdate )
	EVENT( EV_TDM_Lock_OnLockPicked,				CFrobLock::Event_Lock_OnLockPicked )
	EVENT( EV_TDM_Lock_OnLockStatusChange,			CFrobLock::Event_Lock_OnLockStatusChange )
	EVENT( EV_TDM_FrobLock_TriggerTargets,			CFrobLock::Event_TriggerTargets )
	EVENT( EV_TDM_FrobLock_TriggerLockTargets,		CFrobLock::Event_TriggerLockTargets )
	EVENT( EV_TDM_FrobLock_TriggerUnlockTargets,	CFrobLock::Event_TriggerUnlockTargets )
	EVENT( EV_TDM_FrobLock_Open,					CFrobLock::Event_Open )
	EVENT( EV_TDM_FrobLock_Lock,					CFrobLock::Event_Lock )
	EVENT( EV_TDM_FrobLock_Unlock,					CFrobLock::Event_Unlock )
	EVENT( EV_TDM_FrobLock_ToggleLock,				CFrobLock::Event_ToggleLock )
	EVENT( EV_TDM_FrobLock_IsLocked,				CFrobLock::Event_IsLocked )
	EVENT( EV_TDM_FrobLock_IsPickable,				CFrobLock::Event_IsPickable )
	EVENT( EV_TDM_FrobLock_ClearPlayerImmobilization,	CFrobLock::Event_ClearPlayerImmobilization )
END_CLASS

CFrobLock::CFrobLock() :
	m_Lock(NULL),
	m_LastHandleUpdateTime(-1)
{}

CFrobLock::~CFrobLock() 
{
	delete m_Lock;
}

void CFrobLock::AddObjectsToSaveGame(idSaveGame* savefile)
{
	idEntity::AddObjectsToSaveGame(savefile);

	savefile->AddObject(m_Lock);
}

void CFrobLock::Save(idSaveGame *savefile) const
{
	// The lock class is saved by the idSaveGame class on close, no need to handle it here
	savefile->WriteObject(m_Lock);

	savefile->WriteInt(m_Lockhandles.Num());
	for (int i = 0; i < m_Lockhandles.Num(); i++)
	{
		m_Lockhandles[i].Save(savefile);
	}

	savefile->WriteInt(m_LastHandleUpdateTime);
}

void CFrobLock::Restore( idRestoreGame *savefile )
{
	// The lock class is restored by the idRestoreGame, don't handle it here
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Lock));

	int num;
	savefile->ReadInt(num);
	m_Lockhandles.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_Lockhandles[i].Restore(savefile);
	}

	savefile->ReadInt(m_LastHandleUpdateTime);
}

void CFrobLock::Spawn()
{
	m_Lock = static_cast<PickableLock*>(PickableLock::CreateInstance());
	m_Lock->SetOwner(this);
	m_Lock->SetLocked(false);

	// Load the lock spawnargs
	m_Lock->InitFromSpawnargs(spawnArgs);

	// Schedule a post-spawn event to parse the rest of the spawnargs
	// greebo: Be sure to use 16 ms as delay to allow the SpawnBind event to execute before this one.
	PostEventMS(&EV_PostSpawn, 16);
}

void CFrobLock::PostSpawn()
{
	idStr lockHandleName = spawnArgs.GetString("lock_handle", "");
	if (!lockHandleName.IsEmpty())
	{
		idEntity* handleEnt = gameLocal.FindEntity(lockHandleName);

		if (handleEnt != NULL && handleEnt->IsType(CFrobLockHandle::Type))
		{
			// Convert to froblockHandle pointer and call the helper function
			CFrobLockHandle* handle = static_cast<CFrobLockHandle*>(handleEnt);

			AddLockHandle(handle);

			// Check if we should bind the named handle to ourselves
			if (spawnArgs.GetBool("lock_handle_bind_flag", "1"))
			{
				handle->Bind(this, true);
			}
		}
		else
		{
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("lockHandle entity not spawned or of wrong type: %s\r", lockHandleName.c_str());
		}
	}

	// greebo: Should we auto-setup the lockHandles?
	if (spawnArgs.GetBool("auto_setup_lock_handles", "1"))
	{
		AutoSetupLockHandles();
	}
}

void CFrobLock::OnLock()
{
	m_Lock->OnLock();
}

void CFrobLock::OnUnlock()
{
	m_Lock->OnUnlock();
}

void CFrobLock::Lock()
{
	m_Lock->SetLocked(true);

	// Fire the event
	OnLock();
}

void CFrobLock::Unlock()
{
	m_Lock->SetLocked(false);

	// Fire the event
	OnUnlock();
}

void CFrobLock::ToggleLock()
{
	if (IsLocked())
	{
		Unlock();
	}
	else
	{
		Lock();
	}
}

bool CFrobLock::CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) 
{
	// First, check if the frob master can be used
	// If this doesn't succeed, perform additional checks
	idEntity* master = GetFrobMaster();
	if( master != NULL && master->CanBeUsedByItem(item, isFrobUse) )
	{
		return true;
	}

	if (item == NULL) return false;

	assert(item->Category() != NULL);

	const idStr& categoryName = item->Category()->GetName();

	if (categoryName == "#str_02392" ) 			// Keys
	{
		// Keys can always be used on doors
		// Exception: for "frob use" this only applies when the mover is locked
		return (isFrobUse) ? IsLocked() : true;
	}
	else if (categoryName == "#str_02389" ) 		// Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobLock %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks behave similar to keys
		return (isFrobUse) ? IsLocked() : true;
	}

	return false;
}

bool CFrobLock::UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	if (item == NULL) return false;

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
	if (itemEntity == NULL) return false;

	// Get the name of this inventory category
	const idStr& categoryName = item->Category()->GetName();

	if (categoryName == "#str_02392" && impulseState == EPressed ) 			// Keys
	{
		// Keys can be used on button PRESS event, let's see if the key matches
		if (m_UsedByName.FindIndex(itemEntity->name) != -1)
		{
			// just toggle the lock. 
			ToggleLock();
			return true;
		}
		else
		{
			FrobLockStartSound("snd_wrong_key");
			return false;
		}
	}
	else if (categoryName == "#str_02389" ) 					// Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobLock %s is not pickable\r", name.c_str());
			return false;
		}

		// First we check if this item is a lockpick. It has to be of correct type
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
				CancelEvents(&EV_TDM_FrobLock_ClearPlayerImmobilization);
				PostEventMS(&EV_TDM_FrobLock_ClearPlayerImmobilization, 300, playerOwner);
			}

			// Pass the call to the lockpick routine
			return m_Lock->ProcessLockpickImpulse(impulseState, static_cast<int>(str[0]));
		}
		else
		{
			gameLocal.Warning("Wrong 'lockpick_type' spawnarg for lockpicking on item %s, must be a single character.", itemEntity->name.c_str());
			return false;
		}
	}

	return false;
}

void CFrobLock::AttackAction(idPlayer* player)
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

int CFrobLock::FrobLockStartSound(const char* soundName)
{
	// Default implementation: Just play the sound on this entity.
	int length = 0;
	StartSound(soundName, SND_CHANNEL_ANY, 0, false, &length);

	return length;
}

bool CFrobLock::IsLocked()
{
	return m_Lock->IsLocked();
}

bool CFrobLock::IsPickable()
{
	return m_Lock->IsPickable();
}

void CFrobLock::Open()
{
	if (!IsLocked() || cv_door_ignore_locks.GetBool())
	{
		// If we're unlocked, just ToggleOpen the targets
		ToggleOpenTargets();
	}
	else // we're locked
	{
		// If we have handles we want to tap them before the lock starts to open its targets
		if (m_Lockhandles.Num() > 0)
		{
			// Relay the call to the handles, the OpenTargets() call will come back to us
			for (int i = 0; i < m_Lockhandles.Num(); i++)
			{
				CFrobLockHandle* handle = m_Lockhandles[i].GetEntity();
				if (handle == NULL) continue;

				handle->Tap();
			}
		}
		else
		{
			// No handles there to tap, emit the sound
			FrobLockStartSound("snd_locked");
		}
	}
}

void CFrobLock::ToggleOpenTargets()
{
	if (IsLocked() && !cv_door_ignore_locks.GetBool())
	{
		// We're still locked, play the locked sound and exit
		FrobLockStartSound("snd_locked");
	}
	else
	{
		// Actually open any targetted frobmovers
		for ( int i = 0 ; i < targets.Num() ; i++ )
		{
			idEntity* target = targets[i].GetEntity();

			if ( (target == NULL) || !target->IsType(CBinaryFrobMover::Type))
			{
				continue;
			}

			static_cast<CBinaryFrobMover*>(target)->ToggleOpen();
		}
	}
}

void CFrobLock::OpenTargets()
{
	if (IsLocked())
	{
		// We're still locked, play the locked sound and exit
		FrobLockStartSound("snd_locked");
	}
	else
	{
		// Actually open any targetted frobmovers
		for (int i = 0; i < targets.Num(); i++)
		{
			idEntity* target = targets[i].GetEntity();

			if (target == NULL || !target->IsType(CBinaryFrobMover::Type)) continue;

			static_cast<CBinaryFrobMover*>(target)->Open();
		}
	}
}

void CFrobLock::CloseTargets()
{
	// Actually open any targetted frobmovers
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* target = targets[i].GetEntity();

		if (target == NULL || !target->IsType(CBinaryFrobMover::Type)) continue;

		static_cast<CBinaryFrobMover*>(target)->Close();
	}
}

void CFrobLock::LockTargets()
{
	// Lock any targetted frobmovers
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* target = targets[i].GetEntity();

		if (target == NULL || !target->IsType(CBinaryFrobMover::Type)) continue;

		CBinaryFrobMover* mover = static_cast<CBinaryFrobMover*>(target);
		
		if (mover->IsAtClosedPosition())
		{
			mover->Lock();
		}
	}
}

void CFrobLock::UnlockTargets()
{
	// Unlock any targetted frobmovers
	for ( int i = 0 ; i < targets.Num() ; i++ )
	{
		idEntity* target = targets[i].GetEntity();

		if ( (target == NULL) || !target->IsType(CBinaryFrobMover::Type))
		{
			continue;
		}

		static_cast<CBinaryFrobMover*>(target)->Unlock();
	}
}

void CFrobLock::CloseAndLockTargets()
{
	// Close and lock any targetted frobmovers
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* target = targets[i].GetEntity();

		if (target == NULL || !target->IsType(CBinaryFrobMover::Type)) continue;

		static_cast<CBinaryFrobMover*>(target)->CloseAndLock();
	}
}

void CFrobLock::AddLockHandle(CFrobLockHandle* handle)
{
	// Store the pointer and the original position
	idEntityPtr<CFrobLockHandle> handlePtr = handle;

	if (m_Lockhandles.FindIndex(handlePtr) != -1)
	{
		return; // handle is already known
	}

	m_Lockhandles.Append(handlePtr);

	// Let the handle know about us
	handle->SetFrobLock(this);

	// Set up the frob peer relationship between the door and the handle
	m_FrobPeers.AddUnique(handle->name);
	handle->AddFrobPeer(name);
	handle->SetFrobable(m_bFrobable);
}

void CFrobLock::AutoSetupLockHandles()
{
	// Find a suitable teamchain member
	idEntity* part = FindMatchingTeamEntity(CFrobLockHandle::Type);

	while (part != NULL)
	{
		// Found the handle, set it up
		AddLockHandle(static_cast<CFrobLockHandle*>(part));

		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: Auto-added lock handle %s to local list.\r", name.c_str(), part->name.c_str());

		// Get the next handle
		part = FindMatchingTeamEntity(CFrobLockHandle::Type, part);
	}

	for (int i = 0; i < m_Lockhandles.Num(); i++)
	{
		CFrobLockHandle* handle = m_Lockhandles[i].GetEntity();
		if (handle == NULL) continue;

		// The first handle is the master, all others get their master flag set to FALSE
		handle->SetMasterHandle(i == 0);
	}
}

void CFrobLock::UpdateHandlePosition()
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
	for (int i = 0; i < m_Lockhandles.Num(); ++i)
	{
		CFrobLockHandle* handle = m_Lockhandles[i].GetEntity();

		if (handle == NULL) continue;

		handle->SetFractionalPosition(fraction, false);
	}
}

void CFrobLock::Event_Open()
{
	Open();
}

void CFrobLock::Event_Lock_StatusUpdate()
{
	UpdateHandlePosition();
}

void CFrobLock::Event_Lock_OnLockPicked()
{
	Unlock();
}

void CFrobLock::Event_Lock_OnLockStatusChange(int locked)
{
	// Cancel any pending events
	CancelEvents(&EV_TDM_FrobLock_TriggerLockTargets);
	CancelEvents(&EV_TDM_FrobLock_TriggerUnlockTargets);
	CancelEvents(&EV_TDM_FrobLock_TriggerTargets);

	if (locked == 0)
	{
		// Unlocked
		UnlockTargets();
		FrobLockStartSound("snd_unlock");

		if (spawnArgs.GetBool("trigger_targets_on_unlock", "1"))
		{
			// Get the delay for triggering the event
			int delay = spawnArgs.GetInt("unlock_trigger_delay", "0");
			PostEventMS(&EV_TDM_FrobLock_TriggerUnlockTargets, delay);
		}
	}
	else
	{
		// We're locked now
		CloseAndLockTargets();
		FrobLockStartSound("snd_lock");

		if (spawnArgs.GetBool("trigger_targets_on_lock", "1"))
		{
			int delay = spawnArgs.GetInt("lock_trigger_delay", "0");
			PostEventMS(&EV_TDM_FrobLock_TriggerLockTargets, delay);
		}
	}

	// Fire ordinary targets in any case
	if (spawnArgs.GetBool("trigger_targets", "0"))
	{
		int delay = spawnArgs.GetInt("trigger_delay", "0");
		PostEventMS(&EV_TDM_FrobLock_TriggerTargets, delay);
	}
}

void CFrobLock::Event_Lock()
{
	Lock();
}

void CFrobLock::Event_Unlock()
{
	Unlock();
}

void CFrobLock::Event_ToggleLock()
{
	ToggleLock();
}

void CFrobLock::Event_IsLocked()
{
	idThread::ReturnInt(IsLocked());
}

void CFrobLock::Event_IsPickable()
{
	idThread::ReturnInt(IsPickable());
}

void CFrobLock::Event_TriggerTargets()
{
	ActivateTargets(this);

	// Additionally to triggering the targets, update the frobability according to our locked status
	if (spawnArgs.GetBool("update_target_frobability", "0"))
	{
		for (int i = 0; i < targets.Num(); ++i)
		{
			idEntity* ent = targets[i].GetEntity();

			if (ent == NULL) continue;

			// Set frobability based on unlocked/locked status
			ent->SetFrobable(!IsLocked());
		}
	}
}

void CFrobLock::Event_TriggerLockTargets()
{
	bool updateFrobability = spawnArgs.GetBool("update_target_frobability", "0");

	for (const idKeyValue* kv = spawnArgs.MatchPrefix("lock_target"); kv != NULL; kv = spawnArgs.MatchPrefix("lock_target", kv))
	{
		// Find the entity
		idEntity* lockTarget = gameLocal.FindEntity(kv->GetValue());

		if (lockTarget == NULL) 
		{
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Could not find lock target %s (this: %s)\r", kv->GetValue().c_str(), name.c_str());
			continue;
		}

		DM_LOG(LC_LOCKPICK, LT_INFO)LOGSTRING("Activating lock target %s\r", lockTarget->name.c_str());
		lockTarget->Activate(this);

		if (updateFrobability)
		{
			DM_LOG(LC_LOCKPICK, LT_INFO)LOGSTRING("Disabling lock target frobability: %s\r", lockTarget->name.c_str());
			lockTarget->SetFrobable(false);
		}
	}
}

void CFrobLock::Event_TriggerUnlockTargets()
{
	bool updateFrobability = spawnArgs.GetBool("update_target_frobability", "0");

	for (const idKeyValue* kv = spawnArgs.MatchPrefix("unlock_target"); kv != NULL; kv = spawnArgs.MatchPrefix("unlock_target", kv))
	{
		// Find the entity
		idEntity* unlockTarget = gameLocal.FindEntity(kv->GetValue());

		if (unlockTarget == NULL) 
		{
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Could not find unlock target %s (this: %s)\r", kv->GetValue().c_str(), name.c_str());
			continue;
		}

		DM_LOG(LC_LOCKPICK, LT_INFO)LOGSTRING("Activating unlock target %s\r", kv->GetValue().c_str());
		unlockTarget->Activate(this);

		if (updateFrobability)
		{
			DM_LOG(LC_LOCKPICK, LT_INFO)LOGSTRING("Enabling unlock target frobability: %s\r", unlockTarget->name.c_str());
			unlockTarget->SetFrobable(true);
		}
	}
}

void CFrobLock::Event_ClearPlayerImmobilization(idEntity* player)
{
	if (!player->IsType(idPlayer::Type)) return;

	// Release the immobilization imposed on the player by Lockpicking
	static_cast<idPlayer*>(player)->SetImmobilization("Lockpicking", 0);

	// stgatilov #4968: stop lockpicking if player's frob is broken
	// note: release does not look at lockpick type, so we pass garbage
	m_Lock->ProcessLockpickImpulse(EReleased, '-');
}
