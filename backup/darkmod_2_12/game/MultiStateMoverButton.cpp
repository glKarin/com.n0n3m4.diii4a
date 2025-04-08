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



#include "DarkModGlobals.h"
#include "MultiStateMoverButton.h"
#include "MultiStateMover.h"

//===============================================================================
// CMultiStateMoverButton
//===============================================================================

const idEventDef EV_RegisterSelfWithElevator("_MSMBRegisterSelfWithElevator", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_RestoreTargeting("_restoreTargeting", 
	EventArgs('d', "", "", 'd', "", "", 'd', "", ""), EV_RETURNS_VOID, "internal"); // grayman #3029

CLASS_DECLARATION( CFrobButton, CMultiStateMoverButton )
	EVENT( EV_RegisterSelfWithElevator,		CMultiStateMoverButton::Event_RegisterSelfWithElevator)
	EVENT( EV_RestoreTargeting,				CMultiStateMoverButton::Event_RestoreTargeting ) // grayman #3029
END_CLASS

void CMultiStateMoverButton::Spawn()
{
	if (!spawnArgs.GetBool("ride", "0") && !spawnArgs.GetBool("fetch", "0"))
	{
		gameLocal.Warning("Elevator button %s has neither 'fetch' nor 'ride' spawnargs set. AI will not be able to use this button!", name.c_str());
	}
	targetingOff = false;

	PostEventMS(&EV_RegisterSelfWithElevator, 10);
}

void CMultiStateMoverButton::Event_RegisterSelfWithElevator()
{
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* ent = targets[i].GetEntity();

		if (ent == NULL || !ent->IsType(CMultiStateMover::Type))
		{
			continue;
		}

		CMultiStateMover* elevator = static_cast<CMultiStateMover*>(ent);

		// Send the information about us to the elevator
		if (spawnArgs.GetBool("ride", "0"))
		{
			elevator->RegisterButton(this, BUTTON_TYPE_RIDE);
		}

		if (spawnArgs.GetBool("fetch", "0"))
		{
			elevator->RegisterButton(this, BUTTON_TYPE_FETCH);
		}
	}
}

// grayman #3029 - special handling for elevator fetch buttons so
// they can't fetch a moving elevator. The AI's elevator handling
// methods already disallow this, but we need a way to keep the
// player from calling a moving elevator, so the elevator
// mechanics work the same regardless of who's pressing the button.

// If this is a fetch button targetting an elevator, we'll disable
// the targetting for a short time and let the base class handle
// the movement of the button and click sound. Once the button has
// had time to complete its movement, we'll re-enable the targetting.

void CMultiStateMoverButton::ToggleOpen()
{
	if (spawnArgs.GetBool("fetch","0"))
	{
		for ( int i = 0 ; i < targets.Num() ; i++ )
		{
			idEntity* ent = targets[i].GetEntity();

			if (ent == NULL)
			{
				continue;
			}

			const char *classname;
			ent->spawnArgs.GetString("classname", NULL, &classname);
			if (idStr::Cmp(classname, "atdm:mover_elevator") == 0)
			{
				if (!ent->IsAtRest())
				{
					bool toc = false;
					bool too = false;
					bool two = false;

					if (!m_targetingOff) // have we already saved the original flags and turned targeting off?
					{
						// no, so save current targeting
						toc = spawnArgs.GetBool("trigger_on_close", "0");
						too = spawnArgs.GetBool("trigger_on_open", "0");
						two = spawnArgs.GetBool("trigger_when_opened", "0");

						// turn off targeting
						spawnArgs.SetBool( "trigger_on_close", false );
						spawnArgs.SetBool( "trigger_on_open", false );
						spawnArgs.SetBool( "trigger_when_opened", false );
					}

					CBinaryFrobMover::ToggleOpen(); // Pass the call to the base class. This gets us button movement.

					// Restore the saved targeting flags later. This allows the button
					// to open/close, which takes time. 

					if (!m_targetingOff) // have we queued the event already?
					{
						PostEventSec( &EV_RestoreTargeting, 2.0, toc, too, two); // restore 2s from now

						// Set a "targeting turned off" boolean to deal with the case where
						// the player clicks the fetch button before the 2s expires.

						m_targetingOff = true;
					}

					return; // only one elevator per fetch button, so it's safe to return here
				}
				
				break; // only one elevator per fetch button, and we found it
			}
		}
	}

	// Pass the call to the base class, allowing targeting.

	CBinaryFrobMover::ToggleOpen();
}

void CMultiStateMoverButton::Event_RestoreTargeting( bool toc, bool too, bool two)
{
	// restore original targeting

	spawnArgs.SetBool( "trigger_on_close", toc );
	spawnArgs.SetBool( "trigger_on_open", too );
	spawnArgs.SetBool( "trigger_when_opened", two );
	m_targetingOff = false;
}
