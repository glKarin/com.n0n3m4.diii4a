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
#include "PickableLock.h"

static const char* StateNames[] =
{
	"UNLOCKED",
	"LOCKED",
	"LOCKPICKING_STARTED",
	"ADVANCE_TO_NEXT_SAMPLE",
	"PIN_SAMPLE",
	"PIN_SAMPLE_SWEETSPOT",
	"PIN_DELAY",
	"AFTER_PIN_DELAY",
	"WRONG_LOCKPICK_SOUND",
	"PIN_SUCCESS",
	"PIN_FAILED",
	"LOCK_SUCCESS",
	"PICKED",
	"NUM_LPSTATES"
};

// Events to be implemented by owner classes
const idEventDef EV_TDM_Lock_StatusUpdate("TDM_Lock_StatusUpdate", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_TDM_Lock_OnLockPicked("TDM_Lock_OnLockPicked", EventArgs(), EV_RETURNS_VOID, "internal");
// Internal events, no need to expose these to scripts
const idEventDef EV_TDM_Lock_OnLockStatusChange("_TDM_Lock_OnLockStatusChange", 
	EventArgs('d', "state", "1 = locked, 0 = locked"), EV_RETURNS_VOID, "internal");
const idEventDef EV_TDM_LockpickSoundFinished("_TDM_LockpickSoundFinished", 
	EventArgs('d', "nextState", ""), EV_RETURNS_VOID, "internal"); // pass the next state as argument

CLASS_DECLARATION( idClass, PickableLock )
	EVENT( EV_TDM_LockpickSoundFinished,	PickableLock::Event_LockpickSoundFinished )
END_CLASS

PickableLock::PickableLock() :
	m_Owner(NULL),
	m_Locked(false),
	m_LockpickState(NUM_LPSTATES),
	m_FailedLockpickRounds(0),
	m_Pickable(true),
	m_FirstLockedPinIndex(0),
	m_SoundPinSampleIndex(0),
	m_SoundTimerStarted(0)
{}

void PickableLock::SetOwner(idEntity* owner)
{
	m_Owner = owner;
}

void PickableLock::InitFromSpawnargs(const idDict& spawnArgs)
{
	idStr lockPins = spawnArgs.GetString("lock_pins", "");

	// If a door is locked but has no pins, it means it can not be picked and needs a key.
	// In that case we can ignore the pins, otherwise we must create the patterns.
	if (!lockPins.IsEmpty())
	{
		idStr head = "snd_lockpick_pin_";
		int b = cv_lp_pin_base_count.GetInteger();

		if (b < MIN_CLICK_NUM)
		{
			b = MIN_CLICK_NUM;
		}

		for (int i = 0; i < lockPins.Length(); i++)
		{
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pin: %u - %c\r", i, lockPins[i]);

			PinInfo& pin = m_Pins.Alloc();

			char c = lockPins[i];

			int numClicks = 2;

			if (c < '0' || c > '9')
			{
				gameLocal.Warning("Invalid character in pin pattern %s on entity %s, using default: 2", lockPins.c_str(), spawnArgs.GetString("name"));
			}
			else
			{
				numClicks = c - '0';
			}

			pin.pattern = CreatePinPattern(numClicks, b, MAX_PIN_CLICKS, 2, head);

			if (cv_lp_pawlow.GetBool() == false)
			{
				pin.pattern.Insert("snd_lockpick_pin_sweetspot");
			}
			else
			{
				pin.pattern.Append("snd_lockpick_pin_sweetspot");
			}
			
			// Calculate the jiggle position list for this pattern
			// Add one extra position for the delay after the pattern
			pin.positions.SetNum(pin.pattern.Num() + 1);

			// Fill in a linear pattern 
			for (int j = 0; j <= pin.pattern.Num(); ++j)
			{
				pin.positions[j] = j;
			}

			// Random jiggling requires a part of the list to be re-"sorted"
			if (cv_lp_randomize.GetBool() && pin.positions.Num() > 2)
			{
				// Copy the existing pattern
				idList<int> candidates(pin.positions);
				
				// Remove the first and last indices from the candidates, these stay fixed
				candidates.RemoveIndex(0);
				candidates.RemoveIndex(candidates.Num() - 1);

				// Candidates are now in the range [1 .. size(pattern) - 1]

				for (int j = 1; candidates.Num() > 0; ++j)
				{
					// Choose a random candidate and move it to the position list
					int randPos = gameLocal.random.RandomInt(candidates.Num());

					pin.positions[j] = candidates[randPos];

					candidates.RemoveIndex(randPos);
				}
			}
		}
	}

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] pickable (%u)\r", m_Owner->name.c_str(), m_Pickable);

	m_Locked = spawnArgs.GetBool("locked");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] locked (%u)\r", m_Owner->name.c_str(), m_Locked);

	// Initialise the lockpick state
	m_LockpickState = (m_Locked) ? LOCKED : UNLOCKED;
}

bool PickableLock::IsLocked()
{
	return m_Locked;
}

void PickableLock::SetLocked(bool locked)
{
	// For checking whether the status changes
	bool wasLocked = m_Locked;

	// Update to new lock state
	m_Locked = locked;

	if (m_Owner != NULL && wasLocked != locked)
	{
		// Send the lock status change event
		m_Owner->ProcessEvent(&EV_TDM_Lock_OnLockStatusChange, m_Locked);
	}
}

bool PickableLock::IsPickable()
{
	return m_Pickable;
}

void PickableLock::OnLock()
{
	// greebo: Reset the lockpicking values
	m_FirstLockedPinIndex = 0;
	m_SoundTimerStarted = 0;
	m_SoundPinSampleIndex = 0;
	m_FailedLockpickRounds = 0;

	m_LockpickState = LOCKED;
}

void PickableLock::OnUnlock()
{
	m_LockpickState = UNLOCKED;
}

void PickableLock::OnFrobbedStatusChange(bool val)
{
	// stgatilov: this code interrupts lockpicking when frobbed entity switches between door and its handle
	//   https://forums.thedarkmod.com/index.php?/topic/22270-lockpicking-fails-when-handle-crosses-in-front-of-crosshair/
	// we interrupt lockpicking on timeout anyway due to #4968, so this is not necessary

	/*if (val == false)
	{
		// Reset the lockpick fail counter when entity is losing frob focus
		m_FailedLockpickRounds = 0;

		if (m_Locked) {
			// Cancel any pending events
			CancelEvents(&EV_TDM_LockpickSoundFinished);
			// Make sure m_SoundTimerStarted is cleared
			// Also return lock to initial state
			Event_LockpickSoundFinished(LOCKED);
		}
	}*/
}

void PickableLock::OnLockpickPinSuccess()
{
	// Pin was picked, so we try to advance to the next pin.
	m_FirstLockedPinIndex++;

	// If it was the last pin, the user successfully picked the whole lock.
	if (m_FirstLockedPinIndex >= m_Pins.Num())
	{
		if (cv_lp_debug_hud.GetBool())
		{
			idPlayer* player = gameLocal.GetLocalPlayer();
			player->SetGuiString(player->lockpickHUD, "StatusText1", "Lock Successfully picked");
		}

		m_FirstLockedPinIndex = m_Pins.Num();
		m_FailedLockpickRounds = 0;

		// Switch to PICKED state after this sound
		m_LockpickState = LOCK_SUCCESS;

		PropPickSound("snd_lockpick_lock_picked", PICKED);
		
		/*TODO: both EV_TDM_Lock_StatusUpdate and EV_TDM_Lock_OnLockPicked
		  are missing from the BinaryFrobMover. The methods they map to need to be moved from CFrobDoor
		  to CBinaryFrobMover as virtuals
		  Why does this work on mines?*/

		// Move the handle back to its original position
		m_Owner->PostEventMS(&EV_TDM_Lock_StatusUpdate, 32); // Delay by 2 frames as the event aborts if there's < 2 frame 
															 // gap since last update. SteveL #4164, diagnosis by Durandall

		// And unlock the entity after a small delay
		m_Owner->PostEventMS(&EV_TDM_Lock_OnLockPicked, 150);

		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Entity [%s] successfully picked!\r", m_Owner->name.c_str());
	}
	else
	{
		if (cv_lp_debug_hud.GetBool())
		{
			idPlayer* player = gameLocal.GetLocalPlayer();
			player->SetGuiString(player->lockpickHUD, "StatusText1", "Pin Successfully picked");
		}

		m_LockpickState = PIN_SUCCESS;
		m_SoundPinSampleIndex = 0;
		m_FailedLockpickRounds = 0;

		// Fall back to LOCKED state after the sound
		PropPickSound("snd_lockpick_pin_success", LOCKED);

		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Entity [%s] successfully picked!\r", m_Owner->name.c_str());
	}
}

void PickableLock::OnLockpickPinFailure()
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText1", "Pin Failed");
	}

	m_LockpickState = PIN_FAILED;

	m_SoundPinSampleIndex = 0;

	// Fall back to LOCKED state after playing the sound
	PropPickSound("snd_lockpick_pin_fail", LOCKED);

	DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pick attempt: %u/%u failed.\r", m_FirstLockedPinIndex, m_SoundPinSampleIndex);
}

void PickableLock::AttackAction(idPlayer* player)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->CallGui(player->lockpickHUD, "OnAttackButtonPress");
	}

	// Cancel all previous events on attack button hit
	CancelEvents(&EV_TDM_LockpickSoundFinished);
	m_SoundTimerStarted = 0;

	if (m_LockpickState == LOCKED || m_LockpickState == PICKED ||
		m_LockpickState == UNLOCKED)
	{
		// Don't respond to attack actions if not in lockpicking mode in the first place
		return;
	}

	// Check if we're in the "hot spot" of the lock pick sequence
	if (LockpickHotspotActive())
	{
		// Success
		OnLockpickPinSuccess();
	}
	else 
	{
		// Failure
		OnLockpickPinFailure();
	}
}

bool PickableLock::CheckLockpickType(int type)
{
	// Now check if the pick is of the correct type
	idStr pick = m_Owner->spawnArgs.GetString("lock_picktype");

	// Sanity-check the index
	if (m_FirstLockedPinIndex < 0 || m_FirstLockedPinIndex >= pick.Length()) 
	{
		// Incorrect indices
		return false;
	}

	return (pick[m_FirstLockedPinIndex] == '*' || pick[m_FirstLockedPinIndex] == type);
}

bool PickableLock::ProcessLockpickPress(int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText6", "Button Pressed");
	}

	// Check if we're still playing a sound
	if (m_SoundTimerStarted > 0) 
	{
		// Busy, but at least return positive if the lockpick type matches
		return CheckLockpickType(type);
	}

	switch (m_LockpickState)
	{
		case LOCKED:	// Lockpicking not yet started, do so now
		{
			if (CheckLockpickType(type))
			{
				// Start the first sample
				m_LockpickState = LOCKPICKING_STARTED;
				return true;
			}
			else
			{
				if (cv_lp_debug_hud.GetBool())
				{
					idPlayer* player = gameLocal.GetLocalPlayer();
					player->SetGuiString(player->lockpickHUD, "StatusText1", "Wrong Lockpick Type");
				}

				// Fall back to locked after playing the sound
				m_LockpickState = WRONG_LOCKPICK_SOUND;
				PropPickSound("snd_lockpick_pick_wrong", LOCKED, 1000);
				return false;
			}
		}
		case PICKED:	// Already picked or...
		case UNLOCKED:	// ...lockpicking not possible => wrong lockpick sound
		{
			// Play wrong lockpick sound
			m_LockpickState = WRONG_LOCKPICK_SOUND;
			// Fall back to the same state as we're now
			PropPickSound("snd_lockpick_pick_wrong", IsLocked() ? LOCKED : UNLOCKED, 1000);
			return false;
		}
		case WRONG_LOCKPICK_SOUND:
			// Fall back to the same state as we're now
			PropPickSound("snd_lockpick_pick_wrong", IsLocked() ? LOCKED : UNLOCKED, 1000);
			return false;
		case PIN_SAMPLE:
		case ADVANCE_TO_NEXT_SAMPLE:
			// If we encounter a lockpick press during these, reset to start
			m_LockpickState = LOCKPICKING_STARTED;
			return true;
		// Ignore button press events on all other events (while playing sounds, for instance)
		default:
		{
			return true;
		}
	};
}

bool PickableLock::ProcessLockpickRepeat(int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText6", "Button Held Down");
	}

	// Trigger an update event in any case
	m_Owner->ProcessEvent(&EV_TDM_Lock_StatusUpdate);

	// Check if we're still playing a sound
	if (m_SoundTimerStarted > 0) return false; // busy

	bool success = false;

	switch (m_LockpickState)
	{
		case UNLOCKED: // ignore key-repeat events for unlocked doors
		{
			success = false;
			break;
		}
		case LOCKED:	
		{
			// Lockpicking not yet started, start it now
			if (CheckLockpickType(type))
			{
				// Start the first sample
				m_LockpickState = LOCKPICKING_STARTED;
				m_FailedLockpickRounds = 0;
				success = true;
			}
			else
			{
				// Fall back to locked after playing the sound
				m_LockpickState = WRONG_LOCKPICK_SOUND;
				PropPickSound("snd_lockpick_pick_wrong", LOCKED, 500);
				success = false;
			}

			break;
		}
		case LOCKPICKING_STARTED:
		{
			// Initialise the lockpick sample index to -1 and fall through to ADVANCE_TO_NEXT_SAMPLE
			m_SoundPinSampleIndex = -1;
			m_LockpickState = ADVANCE_TO_NEXT_SAMPLE;

			// Fall through to ADVANCE_TO_NEXT_SAMPLE
		}
		case ADVANCE_TO_NEXT_SAMPLE:
		{
			m_SoundPinSampleIndex++;

			const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;

			if (m_SoundPinSampleIndex >= pattern.Num())
			{
				// Switch to the delay after the last sample sound
				m_LockpickState = PIN_DELAY;

				// Use a "nosound" to simulate the silence - then switch back to the first sample
				PropPickSound("snd_silence", AFTER_PIN_DELAY, cv_lp_pick_timeout.GetInteger());

				success = true;
				break;
			}

			// There are more samples to play, are we in pavlov mode and hitting the last sample?
			// Check if this was the last sample or the last but one
			if (m_SoundPinSampleIndex == pattern.Num() - 1 && cv_lp_pawlow.GetBool())
			{
				// greebo: In Pavlov mode, we're entering the hotspot with the beginning
				// of the last sample in the pattern (which is the sweetspot click sound)
				m_LockpickState = PIN_SAMPLE_SWEETSPOT;
			}
			else 
			{
				// Not the last sample, or not in pavlov mode, proceed
				m_LockpickState = PIN_SAMPLE;
			}

			// Fall through
		}
		// Fire the sample sounds
		case PIN_SAMPLE:
		case PIN_SAMPLE_SWEETSPOT:
		{
			// Play the current sample and fall back to ADVANCE_TO_NEXT_SAMPLE
			const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;

			// Sanity-check the sample index
			if (m_SoundPinSampleIndex >= pattern.Num())
			{
				m_LockpickState = ADVANCE_TO_NEXT_SAMPLE; // wrap around
				success = true;
				break;
			}
			
			// Get the pick sound and start playing
			const idStr& pickSound = pattern[m_SoundPinSampleIndex];

			// Pad the sound with a sample delay, for all but the last sample
			int additionalDelay = (m_SoundPinSampleIndex == pattern.Num() - 1) ? 0 : cv_lp_sample_delay.GetInteger();

			PropPickSound(pickSound, ADVANCE_TO_NEXT_SAMPLE, additionalDelay);

			success = true;
			break;
		}
		case PIN_DELAY:
		{
			// We're in delay mode, so ignore this one
			// Either the user hits the hotspot when not in pavlov mode
			// or the "finished sound" event fires and we're back to LOCKPICKING_STARTED
			success = true;
			break;
		}
		case AFTER_PIN_DELAY:
			// The player didn't succeed to get that pin, increase the failed count
			m_FailedLockpickRounds++;

			// greebo: Check if auto-pick should kick in
			if (cv_lp_auto_pick.GetBool()
				&& m_FailedLockpickRounds >= cv_lp_max_pick_attempts.GetInteger())
			{
				OnLockpickPinSuccess();
				success = true;
				break;
			}

			// Goto start
			m_LockpickState = LOCKPICKING_STARTED;

			break;
		case PICKED:	// Already picked
		{
			success = false;
			break;
		}
		// Ignore button press events on all other events (while playing sounds, for instance)
		default:
		{
			success = true;
			break;
		}
	};

	if (cv_lp_debug_hud.GetBool()) 
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText5", StateNames[m_LockpickState]);
	}

	return success;
}

bool PickableLock::ProcessLockpickRelease(int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText6", "Button Released");
	}

	if (m_SoundTimerStarted > 0 && m_LockpickState == WRONG_LOCKPICK_SOUND) 
	{
		return false; // busy playing the wrong lockpick sound
	}
	if (m_LockpickState == LOCKED || m_LockpickState == UNLOCKED || m_LockpickState == LOCK_SUCCESS || m_LockpickState == PIN_SUCCESS)
	{
		//stgatilov #5312: avoid second failed click due to CBinaryFrobMover::Event_ClearPlayerImmobilization
		//also avoid failed click on successful release
		return false;
	}
	
	// Cancel all previous events on release
	CancelEvents(&EV_TDM_LockpickSoundFinished);
	m_SoundTimerStarted = 0;

	// If we're not locked in the first place, don't respond to the release event
	if (!IsLocked())
	{
		return false;
	}

	// Check if we're in the "hot spot" of the lock pick sequence
	if (LockpickHotspotActive())
	{
		// Success
		OnLockpickPinSuccess();
	}
	else
	{
		// Failure
		OnLockpickPinFailure();
		// greebo: Reset the lockpick rounds on failure, the player needs to hold
		// down the key to get the auto-pick feature to kick in
		m_FailedLockpickRounds = 0;
	}

	return true;
}

void PickableLock::UpdateLockpickHUD()
{
	idPlayer* player = gameLocal.GetLocalPlayer();

	player->SetGuiString(player->lockpickHUD, "StatusText2", idStr("Failed lockpick rounds: ") + idStr(m_FailedLockpickRounds));
	player->SetGuiString(player->lockpickHUD, "StatusText4", (idStr("Sounds started: ") + idStr(m_SoundTimerStarted)).c_str());
	player->SetGuiString(player->lockpickHUD, "StatusText5", StateNames[m_LockpickState]);
	player->CallGui(player->lockpickHUD, "OnLockPickProcess");
	player->SetGuiInt(player->lockpickHUD, "HotSpotActive", LockpickHotspotActive() ? 1 : 0);
	player->SetGuiInt(player->lockpickHUD, "HotSpotInActive", LockpickHotspotActive() ? 0 : 1);

	idStr patternText = "Current Pattern: ";
	patternText += idStr(m_FirstLockedPinIndex + 1) + idStr(" of ") + idStr(m_Pins.Num());

	if (m_FirstLockedPinIndex >= m_Pins.Num())
	{
		return; // out of bounds, this can happen right after unlock
	}
	
	const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;
	for (int i = 0; i < pattern.Num(); ++i)
	{
		idStr p = pattern[i];
		p.StripLeadingOnce("snd_lockpick_pin_");

		player->SetGuiString(player->lockpickHUD, "Sample" + idStr(i+1), p);

		float opacity = (i < m_SoundPinSampleIndex) ? 0.2f : 0.6f;

		if (i == m_SoundPinSampleIndex) opacity = 1;

		player->SetGuiFloat(player->lockpickHUD, "SampleOpacity" + idStr(i+1), opacity);
		player->SetGuiInt(player->lockpickHUD, "SampleBorder" + idStr(i+1), (i == m_SoundPinSampleIndex) ? 1 : 0);
	}

	player->SetGuiString(player->lockpickHUD, "PatternText", patternText);
}

bool PickableLock::ProcessLockpickImpulse(EImpulseState impulseState, int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		UpdateLockpickHUD();
	}
	
	switch (impulseState)
	{
		case EPressed:			// just pressed
		{
			return ProcessLockpickPress(type);
		}
		case ERepeat:			// held down
		{
			return ProcessLockpickRepeat(type);
		}
		case EReleased:			// just released
		{
			return ProcessLockpickRelease(type);
		}
		default:
			// Error
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Unhandled impulse state in ProcessLockpick.\r");
			return false;
	};
}

bool PickableLock::LockpickHotspotActive()
{
	if (cv_lp_pawlow.GetBool()) // pavlov mode
	{
		return (m_LockpickState == PIN_SAMPLE_SWEETSPOT);
	}
	else // pattern mode
	{
		return (m_LockpickState == PIN_DELAY);
	}
}

void PickableLock::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_Owner);
	savefile->WriteBool(m_Locked);
	savefile->WriteInt(static_cast<int>(m_LockpickState));
	savefile->WriteInt(m_FailedLockpickRounds);
	
	savefile->WriteInt(m_Pins.Num());
	for (int i = 0; i < m_Pins.Num(); i++)
	{
		// Write the pattern strings
		const idStringList& stringList = m_Pins[i].pattern;

		savefile->WriteInt(stringList.Num());
		for (int j = 0; j < stringList.Num(); j++)
		{
			savefile->WriteString(stringList[j]);
		}

		// Write the positions
		const idList<int>& positions = m_Pins[i].positions;

		savefile->WriteInt(positions.Num());
		for (int j = 0; j < positions.Num(); j++)
		{
			savefile->WriteInt(positions[j]);
		}
	}

	savefile->WriteBool(m_Pickable);
	savefile->WriteInt(m_FirstLockedPinIndex);
	savefile->WriteInt(m_SoundPinSampleIndex);
	savefile->WriteInt(m_SoundTimerStarted);
}

void PickableLock::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Owner));
	savefile->ReadBool(m_Locked);

	int temp;
	savefile->ReadInt(temp);
	assert(temp >= 0 && temp < NUM_LPSTATES);
	m_LockpickState = static_cast<ELockpickState>(temp);

	savefile->ReadInt(m_FailedLockpickRounds);

	int numPins;
	savefile->ReadInt(numPins);
	m_Pins.SetNum(numPins);

	for (int i = 0; i < numPins; i++)
	{
		// Read the pattern
		int num;
		savefile->ReadInt(num);
		m_Pins[i].pattern.SetNum(num);

		for (int j = 0; j < num; j++)
		{
			savefile->ReadString( m_Pins[i].pattern[j] );
		}

		// Read the positions
		savefile->ReadInt(num);
		m_Pins[i].positions.SetNum(num);

		for (int j = 0; j < num; j++)
		{
			savefile->ReadInt( m_Pins[i].positions[j] );
		}
	}

	savefile->ReadBool(m_Pickable);
	savefile->ReadInt(m_FirstLockedPinIndex);
	savefile->ReadInt(m_SoundPinSampleIndex);
	savefile->ReadInt(m_SoundTimerStarted);
}

int PickableLock::PropPickSound(const idStr& picksound, ELockpickState nextState, int additionalDelay)
{
	m_SoundTimerStarted++;

	m_Owner->PropSoundDirect(picksound, true, false, 0.0f, 0); // grayman #3355

	int length = 0;

	// Switch on the owner type - not beautiful, but does the trick without going overboard with virtual functions
	if (m_Owner->IsType(CBinaryFrobMover::Type))
	{
		// Owner is a frobmover, use specialised method
		length = static_cast<CBinaryFrobMover*>(m_Owner)->FrobMoverStartSound(picksound);
	}
	else
	{
		// Ordinary owner entity
		m_Owner->StartSound(picksound, SND_CHANNEL_ANY, 0, false, &length);
	}

	int totalDelay = length + additionalDelay;

	// Post the sound finished event
	PostEventMS(&EV_TDM_LockpickSoundFinished, totalDelay, static_cast<int>(nextState));

	return totalDelay;
}

void PickableLock::Event_LockpickSoundFinished(ELockpickState nextState) 
{
	m_SoundTimerStarted--;

	if (m_SoundTimerStarted < 0) 
	{
		m_SoundTimerStarted = 0;
	}

	// Set the state to the one that was requested
	m_LockpickState = nextState;
}

idStringList PickableLock::CreatePinPattern(int clicks, int baseCount, int maxCount, int strNumLen, const idStr& header)
{
	idStringList returnValue;

	if (clicks < 0 || clicks > 9)
	{
		return returnValue;
	}

	if (clicks == 0)
	{
		clicks = 10;
	}

	clicks += baseCount;
	
	idStr head = va(header + "%%0%uu", strNumLen);

	for (int i = 0; i < clicks; i++)
	{
		int r = gameLocal.random.RandomInt(maxCount + 1);

		idStr click = va(head, r);
		returnValue.Append(click);

		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("PinPattern %u : %s\r", i, click.c_str());
	}

	return returnValue;
}

float PickableLock::CalculateHandleMoveFraction()
{
	if (!IsLocked() || m_LockpickState == LOCK_SUCCESS || m_LockpickState == UNLOCKED || 
		m_LockpickState == PICKED || m_Pins.Num() == 0)
	{
		// unlocked handles or ones without lock pins are at the starting position
		return 0.0f; 
	}

	// Each pin moves the handle by an equal amount
	float pinStep = 1.0f / m_Pins.Num();

	// Calculate the coarse move fraction, depending on the number of unlocked pins
	float fraction = m_FirstLockedPinIndex * pinStep;

	// Sanity-check the pin index before using it as array index
	if (m_FirstLockedPinIndex >= m_Pins.Num()) 
	{
		return fraction;
	}

	// Calculate the fine fraction, based on the current sample number
	const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;

	// Sanity-check the pattern size
	if (pattern.Num() == 0) return fraction;

	float sampleStep = pinStep / pattern.Num();

	int curSampleIndex = m_Pins[m_FirstLockedPinIndex].positions[m_SoundPinSampleIndex];

	// During the delay, the handle is using the last position index
	if (m_LockpickState == PIN_DELAY) 
	{
		curSampleIndex = m_Pins[m_FirstLockedPinIndex].positions.Num() - 1;
	}

	// Add the fine movement fraction
	fraction += curSampleIndex * sampleStep;

	// Clamp the fraction to reasonable values
	fraction = idMath::ClampFloat(0.0f, 1.0f, fraction);

	return fraction;
}
