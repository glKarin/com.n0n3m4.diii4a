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



#include "ConversationState.h"
#include "../Memory.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/PlayAnimationTask.h"
#include "../Tasks/InteractionTask.h"
#include "../Tasks/ScriptTask.h"
#include "ObservantState.h"
#include "../Library.h"
#include "../Subsystem.h"
#include "../Conversation/Conversation.h"
#include "../Conversation/ConversationSystem.h"
#include "../Conversation/ConversationCommand.h"

// greebo: This spawnarg holds the currently played conversation sound
#define CONVERSATION_SPAWNARG "snd_TEMP_conv"
#define DEFAULT_LOOKAT_DURATION 5.0f
#define DEFAULT_WALKTOENTITY_DISTANCE 50.0f
#define FALLBACK_ANIM_LENGTH 5000 // msecs
#define DEFAULT_BLEND_FRAMES 4

namespace ai
{

ConversationState::ConversationState() :
	_conversation(-1),
	_state(ENotReady),
	_commandType(ConversationCommand::ENumCommands),
	_finishTime(-1)
{}

// Get the name of this state
const idStr& ConversationState::GetName() const
{
	static idStr _name(STATE_CONVERSATION);
	return _name;
}

void ConversationState::Cleanup(idAI* owner) // grayman #3559
{
	owner->m_InConversation = false;
}

// Wrap up and end state

void ConversationState::Wrapup(idAI* owner)
{
	Cleanup(owner);
	owner->GetMind()->EndState();
}

bool ConversationState::CheckAlertLevel(idAI* owner)
{
	// Alert index is too high for index > ERelaxed
	return (owner->AI_AlertIndex <= ESuspicious); // grayman #3449 - was ERelaxed
}

void ConversationState::SetConversation(int index)
{
	if (gameLocal.m_ConversationSystem->GetConversation(index) == NULL)
	{
		gameLocal.Warning("AI ConversationState: Could not find conversation %d", index);
	}

	_conversation = index;
}

void ConversationState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ConversationState initialised.\r");
	assert(owner);

	// Memory shortcut
	Memory& memory = owner->GetMemory();
	memory.alertClass = EAlertNone;
	memory.alertType = EAlertTypeNone;

	_alertLevelDecreaseRate = 0.01f;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s is too alert to start a conversation\r", owner->GetName());
		return;
	}

	// Check dialogue prerequisites
	if (!CheckConversationPrerequisites())
	{
		Wrapup(owner); // grayman #3559
		//owner->GetMind()->EndState();
		return;
	}

	owner->actionSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->movementSubsystem->ClearTasks();
	owner->searchSubsystem->ClearTasks(); // grayman #3857

	owner->StopMove(MOVE_STATUS_DONE);
	memory.StopReacting(); // grayman #3559

	ConversationPtr conversation = gameLocal.m_ConversationSystem->GetConversation(_conversation);
	if (conversation == NULL)
	{
		Wrapup(owner); // grayman #3559
		//owner->GetMind()->EndState();
		return;
	}

	// We're initialised, so let's set this state to ready
	_state = EReady;

	// Check the conversation property to see if we should move before we are ready
	if (conversation->ActorsMustBeWithinTalkdistance())
	{
		// Not ready yet
		_state = ENotReady;

		idEntity* targetActor = NULL;

		// Get the first actor who is not <self>
		for (int i = 0; i < conversation->GetNumActors(); i++)
		{
			idEntity* candidate = conversation->GetActor(i);
			if (candidate != owner)
			{
				targetActor = candidate;
				break;
			}
		}

		if (targetActor != NULL)
		{
			float talkDistance = conversation->GetTalkDistance();

			owner->movementSubsystem->PushTask(
				TaskPtr(new MoveToPositionTask(targetActor, talkDistance))
			);
		}
	}
}

// Gets called each time the mind is thinking
void ConversationState::Think(idAI* owner)
{
	//Memory& memory = owner->GetMemory();

	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) 
	{
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s is too alert to continue a conversation\r", owner->GetName());
		owner->GetMind()->SwitchState(owner->backboneStates[EObservant]);
		return;
	}

	// Let the AI check its senses
	owner->PerformVisualScan();

	if ( ( _finishTime > 0 ) && ( gameLocal.time > _finishTime ) )
	{
		// Allow new incoming commands
		_state = EReady;

		// Reset the finish time
		_finishTime = -1;
	}

	DrawDebugOutput(owner);
}

ConversationState::ExecutionState ConversationState::GetExecutionState()
{
	return _state;
}

bool ConversationState::CheckConversationPrerequisites()
{
	// TODO
	return true;
}

void ConversationState::OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
{
	if (_state != EExecuting && _state != ENotReady && _state != EBusy) return;

	if (subSystem == SubsysMovement)
	{
		// greebo: Are we still in preparation phase?
		if (_state == ENotReady)
		{
			// The movement task has ended, set the state to ready
			_state = EReady;
			return;
		}

		// In case of active "walk" commands, set the state to ready
		if (_commandType == ConversationCommand::EWalkToEntity || 
			_commandType == ConversationCommand::EWalkToPosition || 
			_commandType == ConversationCommand::EWalkToActor)
		{
			// Check if the subsystem is actually empty
			if (owner->GetSubsystem(subSystem)->IsEmpty())
			{
				_state = EReady; // ready for new commands
				return;
			}
		}
	}
	else if (subSystem == SubsysAction)
	{
		// In case of active "Interact" commands, set the state to "finished"
		if (_commandType == ConversationCommand::EInteractWithEntity || _commandType == ConversationCommand::ERunScript)
		{
			_state = EReady;
			return;
		}
	}
}

void ConversationState::StartCommand(ConversationCommand& command, Conversation& conversation)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	switch (command.GetType())
	{
	case ConversationCommand::EWaitSeconds:
		if (!idStr::IsNumeric(command.GetArgument(0)))
		{
			gameLocal.Warning("Conversation Command argument for 'WaitSeconds' is not numeric: %s", command.GetArgument(0).c_str());
		}
		_finishTime = gameLocal.time + SEC2MS(atof(command.GetArgument(0)));
		_state = EBusy; // block new commands until finished
	break;

	case ConversationCommand::EWalkToActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			float distance = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_WALKTOENTITY_DISTANCE;
			
			owner->movementSubsystem->PushTask(
				TaskPtr(new MoveToPositionTask(ai, distance))
			);

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'WalkToActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	case ConversationCommand::EWalkToPosition:
	{
		idVec3 goal = command.GetVectorArgument(0);

		// Start moving
		owner->movementSubsystem->PushTask(
			TaskPtr(new MoveToPositionTask(goal))
		);

		// Check if we should wait until the command is finished and set the _state accordingly
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::EWalkToEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);
		if (ent != NULL)
		{
			float distance = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_WALKTOENTITY_DISTANCE;
			
			owner->movementSubsystem->PushTask(
				TaskPtr(new MoveToPositionTask(ent, distance))
			);

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'WalkToEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EStopMove:
		owner->StopMove(MOVE_STATUS_DONE);
		_state = EReady;
		break;

	case ConversationCommand::ETalk:
	{
		int length = Talk(owner, command.GetArgument(0));
		
		// Check if we need to look at the listener
		if (conversation.ActorsAlwaysFaceEachOtherWhileTalking())
		{
			idAI* talker = owner;

			for (int i = 0; i < conversation.GetNumActors(); i++)
			{
				if (i != command.GetActor())
				{
					// Listeners turn towards the talker
					// Reduce the actor index by 1 before passing them to the conversation
					idAI* listener = conversation.GetActor(i);

					listener->TurnToward(talker->GetEyePosition());
					listener->Event_LookAtPosition(talker->GetEyePosition(), MS2SEC(length));

					// Listeners are idle
					listener->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 8);
				}
				else
				{
					// The talker should turn to any other listener

					// Are there any other actors at all?
					if (conversation.GetNumActors() > 1) 
					{
						idAI* listener = (i == 0) ? conversation.GetActor(1) : conversation.GetActor(0);

						talker->TurnToward(listener->GetEyePosition());
						talker->Event_LookAtPosition(listener->GetEyePosition(), MS2SEC(length));
						
						// Let the talker do his animation
						talker->SetAnimState(ANIMCHANNEL_TORSO, "Torso_IdleTalk", 8);
					}
				}
			}
		}
		
		_finishTime = gameLocal.time + length + 200;
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::EPlayAnimOnce:
	{
		idStr animName = command.GetArgument(0);
		int blendFrames = (command.GetNumArguments() >= 2) ? atoi(command.GetArgument(1)) : DEFAULT_BLEND_FRAMES;

		// Tell the animation subsystem to play the anim
		owner->actionSubsystem->PushTask(TaskPtr(new PlayAnimationTask(animName, blendFrames)));

		int length = (owner->GetAnimator() != NULL) ? 
			owner->GetAnimator()->AnimLength(owner->GetAnimator()->GetAnim(animName)) : FALLBACK_ANIM_LENGTH;

		// Set the finish conditions for the current action
		_finishTime = gameLocal.time + length;
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::EPlayAnimCycle:
	{
		idStr animName = command.GetArgument(0);
		int blendFrames = (command.GetNumArguments() >= 2) ? atoi(command.GetArgument(1)) : DEFAULT_BLEND_FRAMES;

		// Tell the animation subsystem to play the anim
		owner->actionSubsystem->PushTask(TaskPtr(new PlayAnimationTask(animName, blendFrames, true))); // true == playCycle

		// For PlayCycle, "wait until finished" doesn't make sense, as it lasts forever
		_state = EReady;
	}
	break;

	case ConversationCommand::EActivateTarget:
	{
		idEntity* ent = command.GetEntityArgument(0);
		if (ent != NULL)
		{
			// Post a trigger event
			ent->PostEventMS(&EV_Activate, 0, owner);
			// We're done
			_state = EReady;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'ActivateTarget' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	case ConversationCommand::ELookAtActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;
			owner->Event_LookAtEntity(ai, duration);

			_finishTime = gameLocal.time + SEC2MS(duration);
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'LookAtActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ELookAtPosition:
	{
		idVec3 pos = command.GetVectorArgument(0);
		float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;

		owner->Event_LookAtPosition(pos, duration);

		_finishTime = gameLocal.time + SEC2MS(duration);
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::ELookAtEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;
			owner->Event_LookAtEntity(ent, duration);
			
			_finishTime = gameLocal.time + SEC2MS(duration);
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'LookAtEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ETurnToActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			owner->TurnToward(ai->GetEyePosition());
			_state = EReady;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'TurnToActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ETurnToPosition:
	{
		idVec3 pos = command.GetVectorArgument(0);
		owner->TurnToward(pos);
		_state = EReady;
	}
	break;
	
	case ConversationCommand::ETurnToEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			owner->TurnToward(ent->GetPhysics()->GetOrigin());
			_state = EReady;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'TurnToEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	
	case ConversationCommand::EAttackActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			owner->SetEnemy(ai);
			owner->SetAlertLevel(owner->thresh_5 + 1);
			_state = EReady;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'AttackActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EAttackEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL && ent->IsType(idActor::Type))
		{
			owner->SetEnemy(static_cast<idActor*>(ent));
			owner->SetAlertLevel(owner->thresh_5 + 1);
			_state = EReady;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'AttackEntity' could not find entity or entity is of wrong type: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EInteractWithEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			// Tell the action subsystem to do its job
			owner->actionSubsystem->PushTask(TaskPtr(new InteractionTask(ent)));

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'InteractWithEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ERunScript:
	{
		idStr scriptFunction = command.GetArgument(0);

		if (!scriptFunction.IsEmpty())
		{
			// Tell the action subsystem to do its job
			owner->actionSubsystem->PushTask(TaskPtr(new ScriptTask(scriptFunction)));

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'RunScript' has empty scriptfunction argument 0.");
		}
	}
	break;

	case ConversationCommand::EWaitForActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		ExecutionState otherState = GetConversationStateOfActor(ai); // it's safe to pass NULL pointers

		if (otherState == EExecuting || otherState == EBusy)
		{
			// The other actor is executing, set ourselves to "busy"
			_state = EBusy;
		}
		else
		{
			// The other actor is not executing or busy, we've got nothing to do
			_state = EReady;
		}
	}
	break;

	default:
		gameLocal.Warning("Unknown command type found %d", command.GetType());
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Unknown command type found %d", command.GetType());
		_state = EReady;
	};

	// Store the command type
	_commandType = command.GetType();
}

void ConversationState::ProcessCommand(ConversationCommand& command)
{
	ConversationPtr conversation = gameLocal.m_ConversationSystem->GetConversation(_conversation);

	if (conversation == NULL) return;

	// Check the incoming command
	ConversationCommand::State cmdState = command.GetState();

	if (cmdState == ConversationCommand::EReadyForExecution)
	{
		// This is a new command, are we ready for it?
		if (_state == EReady || _state == EExecuting)
		{
			// Yes, we're able to handle new commands
			StartCommand(command, *conversation);
		}
		else
		{
			// Not ready for new commands yet, wait...
		}
	}
	else if (cmdState == ConversationCommand::EExecuting)
	{
		// We are already executing this command, continue
		Execute(command, *conversation);
	}
	else
	{
		// Ignore the other cases
	}

	// Now update the command state, based on our execution state
	switch (_state)
	{
		case ENotReady:
			// not ready yet, state is still preparing for takeoff
			// don't change the command
			break;
		case EReady:
			command.SetState(ConversationCommand::EFinished);
			break;
		case EExecuting:
			// We're executing the command, but it's basically done.
			command.SetState(ConversationCommand::EFinished);
			break;
		case EBusy:
			// We're executing the command, and must wait until it's finished, set it to "executing"
			command.SetState(ConversationCommand::EExecuting);
			break;
		default:
			// Unknown state?
			gameLocal.Warning("Unknown execution state found: %d", static_cast<int>(_state));
			// Set the command to finished anyway, to avoid blocking
			command.SetState(ConversationCommand::EFinished);
			break;
	};
}

ConversationState::ExecutionState ConversationState::GetConversationStateOfActor(idAI* ai)
{
	if (ai != NULL)
	{
		// Get the actor's conversationstate
		ConversationStatePtr convState = std::dynamic_pointer_cast<ConversationState>(ai->GetMind()->GetState());

		// Get the state and exit
		return (convState != NULL) ? convState->GetExecutionState() : ENumExecutionStates;
	}
	
	return ENumExecutionStates;
}

void ConversationState::Execute(ConversationCommand& command, Conversation& conversation)
{
	switch (command.GetType())
	{
		case ConversationCommand::EWaitForActor:
		{
			// Reduce the actor index by 1 before passing them to the conversation
			idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

			ExecutionState otherState = GetConversationStateOfActor(ai);

			if (otherState != EExecuting && otherState != EBusy)
			{
				// The other actor is not executing or busy anymore, we're done waiting
				_state = EReady;
			}
		}

		// All other cases are not relevant right now
		default:
			break;
	};
}

int ConversationState::Talk(idAI* owner, const idStr& soundName)
{
	const idKeyValue* kv = owner->spawnArgs.FindKey(soundName);

	if (kv != NULL && kv->GetValue().Icmpn( "snd_", 4 ) == 0)
	{
		// The conversation argument is pointing to a valid spawnarg on the owner
		owner->spawnArgs.Set(CONVERSATION_SPAWNARG, kv->GetValue());
	}
	else
	{
		// The spawnargs don't define the sound shader, set the shader directly
		owner->spawnArgs.Set(CONVERSATION_SPAWNARG, soundName);
	}

	// Start the sound
	int length = owner->PlayAndLipSync(CONVERSATION_SPAWNARG, "talk1", 0); // grayman #3355

	// Clear the spawnarg again
	owner->spawnArgs.Set(CONVERSATION_SPAWNARG, "");

	return length;
}

void ConversationState::DrawDebugOutput(idAI* owner)
{
	if (!cv_ai_show_conversationstate.GetBool()) return;

	idStr str;

	switch (_state)
	{
		case ENotReady: str = "Not Ready"; break;
		case EReady: str = "Ready"; break;
		case EExecuting: str = "Executing"; break;
		case EBusy: str = "Busy"; break;
		default:break;
	};

	gameRenderWorld->DebugText(str, owner->GetEyePosition() - idVec3(0,0,20), 0.25f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis, 1, 48);

	str = (_commandType < ConversationCommand::ENumCommands) ? ConversationCommand::TypeNames[_commandType] : "";
	gameRenderWorld->DebugText(str, owner->GetEyePosition() - idVec3(0,0,10), 0.3f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis, 1, 48);
}

// angua: override visual stim to avoid greetings during conversation
void ConversationState::OnActorEncounter(idEntity* stimSource, idAI* owner)
{
	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	//Memory& memory = owner->GetMemory();

	if (!stimSource->IsType(idActor::Type)) return; // No Actor, quit

	// Hard-cast the stimsource onto an actor 
	idActor* other = static_cast<idActor*>(stimSource);	

	// Are they dead or unconscious?
	if (other->health <= 0 || other->IsKnockedOut() || owner->IsEnemy(other))
	{
		State::OnActorEncounter(stimSource, owner);
	}
}

void ConversationState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_conversation);
	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(static_cast<int>(_commandType));
	savefile->WriteInt(_finishTime);
}

void ConversationState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_conversation);

	int temp;
	savefile->ReadInt(temp);
	assert(temp >= 0 && temp <= ENumExecutionStates); // sanity check
	_state = static_cast<ExecutionState>(temp);

	savefile->ReadInt(temp);
	assert(temp >= 0 && temp <= ConversationCommand::ENumCommands); // sanity check
	_commandType = static_cast<ConversationCommand::Type>(temp);

	savefile->ReadInt(_finishTime);
}

StatePtr ConversationState::CreateInstance()
{
	return StatePtr(new ConversationState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar conversationStateRegistrar(
	STATE_CONVERSATION, // Task Name
	StateLibrary::CreateInstanceFunc(&ConversationState::CreateInstance) // Instance creation callback
);

} // namespace ai
