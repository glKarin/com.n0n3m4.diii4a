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



#include "GreetingBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

GreetingBarkTask::GreetingBarkTask() :
	SingleBarkTask()
{}

GreetingBarkTask::GreetingBarkTask(const idStr& soundName, idActor* greetingTarget, bool isInitialGreeting) : // grayman #3415
	SingleBarkTask(soundName),
	_greetingTarget(greetingTarget),
	_isInitialGreeting(isInitialGreeting) // grayman #3415
{}

// Get the name of this task
const idStr& GreetingBarkTask::GetName() const
{
	static idStr _name(TASK_GREETING_BARK);
	return _name;
}

void GreetingBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	SingleBarkTask::Init(owner, subsystem);

	// Check the prerequisites - are both AI available for greeting?
	if ( !owner->CanGreet() || !_greetingTarget->CanGreet() || owner->m_isMute ) // grayman #3338
	{
		// Owner or other AI cannot do greetings
		subsystem.FinishTask();
		return;
	}

	// Allow state "waiting for greeting" for owner
	// Allow state "after Greeting" for the other AI
	if ( ( ( owner->greetingState != ENotGreetingAnybody )           && ( owner->greetingState != EWaitingForGreeting ) ) || 
		 ( ( _greetingTarget->greetingState != ENotGreetingAnybody ) && ( _greetingTarget->greetingState != EAfterGreeting ) ) )
	{
		// Someone is busy
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Cannot greet: one of the actors is busy: %s to %s\r", owner->name.c_str(), _greetingTarget->name.c_str());
		subsystem.FinishTask();
		return;
	}

	// grayman #3415 - If both actors are sitting, disallow a greeting, on the
	// assumption that they've been that way for awhile, and any greetings would
	// already have happened. To keep pent-up greetings from occurring when one
	// actor stands up, move the allowed greeting time into the future.

	idAI* otherAI = static_cast<idAI*>(_greetingTarget);
	if ( (owner->GetMoveType() == MOVETYPE_SIT ) && (otherAI->GetMoveType() == MOVETYPE_SIT ) )
	{
		int delay = (MINIMUM_TIME_BETWEEN_GREETING_SAME_ACTOR + gameLocal.random.RandomInt(EXTRA_DELAY_BETWEEN_GREETING_SAME_ACTOR))*1000;
		int nextGreetingTime = gameLocal.time + delay;
		owner->GetMemory().GetGreetingInfo(otherAI).nextGreetingTime = nextGreetingTime;
		otherAI->GetMemory().GetGreetingInfo(owner).nextGreetingTime = nextGreetingTime;
		subsystem.FinishTask();
		return;
	}

	// Both AI are not greeting each other so far, continue
	owner->greetingState = EGoingToGreet;
	_greetingTarget->greetingState = EWaitingForGreeting;
}

bool GreetingBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("GreetingBarkTask performing.\r");

	// Let the SingleBarkTask do the timing and act upon the result
	bool done = SingleBarkTask::Perform(subsystem);

	// Set the Greeting State according to our base class' work
	idAI* owner = _owner.GetEntity();

	if (done)
	{
		owner->greetingState = EAfterGreeting;

		if ( ( _greetingTarget != NULL ) && _greetingTarget->IsType(idAI::Type) )
		{
			idAI* otherAI = static_cast<idAI*>(_greetingTarget);

			if ( _isInitialGreeting ) // grayman #3415 - only send a msg if you're the initiator
			{

				CommMessage message(
					CommMessage::Greeting_CommType, 
					owner, otherAI, // from this AI to the other
					NULL,
					owner->GetPhysics()->GetOrigin(),
					0
				);

				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Sending AI Comm Message to %s.\r", otherAI->name.c_str());
				otherAI->GetMind()->GetState()->OnAICommMessage(message, 0);
			}

			// grayman #3415
			// Establish next time owner can greet otherAI.

			int delay = (MINIMUM_TIME_BETWEEN_GREETING_SAME_ACTOR + gameLocal.random.RandomInt(EXTRA_DELAY_BETWEEN_GREETING_SAME_ACTOR))*1000;
			owner->GetMemory().GetGreetingInfo(otherAI).nextGreetingTime = gameLocal.time + delay;
		}

		// Owner is done here
	}
	else if (_endTime < 0)
	{
		// Greeting not yet dispatched, waiting
		owner->greetingState = EGoingToGreet;
	}
	else 
	{
		if ( owner->greetingState == EGoingToGreet )
		{
			if (cv_ai_debug_greetings.GetBool())
			{
				gameLocal.Printf("%s barks '%s' to %s\n",owner->GetName(),_soundName.c_str(),_greetingTarget->GetName());
				if ( !_isInitialGreeting )
				{
					gameLocal.Printf("\n");
				}
			}
		}

		// End time is set, we're currently barking
		owner->greetingState = EIsGreeting;

		int timeLeft = _endTime - gameLocal.time;

		if (timeLeft > 32)
		{
			owner->Event_LookAtPosition(_greetingTarget->GetEyePosition(), MS2SEC(timeLeft));
		}
	}

	if ( ( _barkStartTime > 0 ) && ( gameLocal.time > _barkStartTime + 50000 ) )
	{
		gameLocal.Printf("Force termination of GreetingBarkTask after 50 seconds: %s.\n", owner->name.c_str());
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("Force termination of GreetingBarkTask after 50 seconds: %s.\r", owner->name.c_str());
		return true;
	}

	return done;
}

void GreetingBarkTask::OnFinish(idAI* owner)
{
	if ( owner )
	{
		if ( owner->greetingState != ECannotGreet )
		{
			owner->greetingState = ENotGreetingAnybody;
		}

		// grayman #3415 - If this was a response, no further action from the target AI

		if ( !_isInitialGreeting )
		{
			if ( _greetingTarget->greetingState != ECannotGreet )
			{
				_greetingTarget->greetingState = ENotGreetingAnybody;
			}
		}
	}
}

// Save/Restore methods
void GreetingBarkTask::Save(idSaveGame* savefile) const
{
	SingleBarkTask::Save(savefile);

	savefile->WriteObject(_greetingTarget);
	savefile->WriteBool(_isInitialGreeting); // grayman #3415
}

void GreetingBarkTask::Restore(idRestoreGame* savefile)
{
	SingleBarkTask::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_greetingTarget));
	savefile->ReadBool(_isInitialGreeting); // grayman #3415
}

GreetingBarkTaskPtr GreetingBarkTask::CreateInstance()
{
	return GreetingBarkTaskPtr(new GreetingBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar greetingBarkTaskRegistrar(
	TASK_GREETING_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&GreetingBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
