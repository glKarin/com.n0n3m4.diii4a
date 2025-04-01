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



#include <climits>
#include "Conversation.h"
#include "../States/ConversationState.h"
#include "../Memory.h"

namespace ai {

const int MAX_ALERT_LEVEL_TO_START_CONVERSATION = ESuspicious; // grayman #3449 - was '1'

Conversation::Conversation() :
	_isValid(false),
	_talkDistance(0.0f),
	_playCount(0),
	_maxPlayCount(-1),
	_actorsMustBeWithinTalkDistance(true),
	_actorsAlwaysFaceEachOtherWhileTalking(true)
{}

Conversation::Conversation(const idDict& spawnArgs, int index) :
	_isValid(false),
	_talkDistance(0.0f),
	_playCount(0),
	_maxPlayCount(-1),
	_actorsMustBeWithinTalkDistance(true),
	_actorsAlwaysFaceEachOtherWhileTalking(true)
{
	// Pass the call to the parser
	InitFromSpawnArgs(spawnArgs, index);
}

bool Conversation::IsValid()
{
	return _isValid;
}

const idStr& Conversation::GetName() const
{
	return _name;
}

int Conversation::GetPlayCount()
{
	return _playCount;
}

int Conversation::GetMaxPlayCount()
{
	return _maxPlayCount;
}

bool Conversation::ActorsMustBeWithinTalkdistance()
{
	return _actorsMustBeWithinTalkDistance;
}

bool Conversation::ActorsAlwaysFaceEachOtherWhileTalking()
{
	return _actorsAlwaysFaceEachOtherWhileTalking;
}

float Conversation::GetTalkDistance()
{
	return _talkDistance;
}

bool Conversation::CheckConditions()
{
	// Check if the max play count has been exhausted already
	if (_maxPlayCount != -1 && _playCount >= _maxPlayCount)
	{
		// play count exhausted
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Conversation has exceeded the maximum playcount (current playcount = %d)!.\r", _name.c_str(), _playCount);
		return false;
	}

	// greebo: Check if all actors are available
	for (int i = 0; i < _actors.Num(); i++)
	{
		// Get the actor
		idAI* ai = GetActor(i);

		if (ai == NULL || ai->IsKnockedOut() || ai->health <= 0)
		{
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s in conversation %s is KO or dead!\r", _actors[i].c_str(), _name.c_str());
			return false;
		}

		if (ai->AI_AlertIndex > MAX_ALERT_LEVEL_TO_START_CONVERSATION)
		{
			// AI is too alerted to start this conversation
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s in conversation %s is too alert to start this conversation\r", _actors[i].c_str(), _name.c_str());
			return false;
		}

		// greebo: Let's see if the AI has already an active conversation state
		// grayman #3559 - use simpler method of checking
		if (ai->m_InConversation)
		{
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s is already in another conversation!\r", ai->name.c_str());
			return false;
		}
/* previous code
		// FIXME: This might not be enough, if the AI has pushed other states on top of the conversation state
		ConversationStatePtr convState = 
				std::dynamic_pointer_cast<ConversationState>(ai->GetMind()->GetState());

		if (convState != NULL)
		{
			// AI is already involved in another conversation
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s is already in another conversation!\r", ai->name.c_str());
			return false;
		}
 */
	}

	// All actors alive and kicking

	// TODO: Other checks to perform?

	return true;
}

void Conversation::Start()
{
	// Switch all participating AI to conversation state
	for ( int i = 0 ; i < _actors.Num() ; i++ )
	{
		idAI* ai = GetActor(i);

		if (ai == NULL)
		{
			continue;
		}
		
		ai->SwitchToConversationState(_name);
		ai->m_InConversation = true; // grayman #3559
	}

	_playCount++;

	// Set the index to the first command
	_currentCommand = 0;

	// Reset the command execution status to ready
	for ( int i = 0 ; i < _commands.Num() ; i++ )
	{
		_commands[i]->SetState(ConversationCommand::EReadyForExecution);
	}
}

void Conversation::End()
{
	// Switch all participating AI out of conversation state
	for ( int i = 0 ; i < _actors.Num() ; i++ )
	{
		idAI* ai = GetActor(i);

		if (ai == NULL)
		{
			continue;
		}

		// Let's see if the AI is currently in a conversation.
		ConversationStatePtr convState = 
			std::dynamic_pointer_cast<ConversationState>(ai->GetMind()->GetState());

		if (convState != NULL)
		{
			// End the conversation state
			ai->GetMind()->EndState();
			ai->m_InConversation = false; // grayman #3559
		}
	}
}

bool Conversation::IsDone()
{
	return _currentCommand >= _commands.Num();
}

bool Conversation::AllActorsReady()
{
	// Cycle through all actors and return false as soon as a busy one is detected
	for (int i = 0; i < GetNumActors(); i++)
	{
		ConversationStatePtr convState = GetConversationState(i);

		if (convState == NULL)
		{
			// This is the pathological case, but still don't return false here, 
			// otherwise the conversation is blocked
			continue;
		}

		// Only continue the loop if this actor is in ready state
		if (convState->GetExecutionState() != ConversationState::EReady)
		{
			return false;
		}
	}

	return true;
}

bool Conversation::CheckActorAvailability()
{
	// Cycle through all actors and return false as soon as a busy one is detected
	for (int i = 0; i < GetNumActors(); i++)
	{
		idAI* actor = GetActor(i);

		if (actor == NULL) return false; // actor removed from game!

		if (actor->IsKnockedOut() || actor->AI_DEAD) return false;
	}

	return true; // all checks passed
}

bool Conversation::Process()
{
	// Check for index out of bounds in debug builds
	assert(_currentCommand >= 0 && _currentCommand < _commands.Num());

	// Get the command as specified by the pointer
	const ConversationCommandPtr& command = _commands[_currentCommand];

	// greebo: Check if any of the actors has been knocked out or killed
	if (!CheckActorAvailability())
	{
		return false;
	}

	if (command->GetType() == ConversationCommand::EWaitForAllActors)
	{
		// Special case: we need to wait until all actors are ready again
		if (AllActorsReady())
		{
			// Done, all actors are in "ready" state
			command->SetState(ConversationCommand::EFinished);
			// Proceed to the next command
			_currentCommand++;
		}
		else
		{
			command->SetState(ConversationCommand::EExecuting);
		}
	}
	else
	{
		// Normal operation: let the current actor perform its task
		ConversationStatePtr convState = GetConversationState(command->GetActor());

		if (convState != NULL)
		{
			// greebo: Pass the command to the conversation state for processing
			convState->ProcessCommand(*command);
		}
		else
		{
			return false;
		}

		// Get the state of this command to decide on further actions
		ConversationCommand::State state = command->GetState();
		
		switch (state)
		{
			case ConversationCommand::EReadyForExecution:
				// not been processed yet, state is still preparing for takeoff
				break;
			case ConversationCommand::EExecuting:
				// Is being processed, do nothing
				break;
			case ConversationCommand::EFinished:
			case ConversationCommand::EAborted:
				// Command is done processing, increase the iterator, we continue next frame
				_currentCommand++;
				break;
			default:
				return false;
		};
	}

	// Return TRUE if the command iterator is still in the valid range (i.e. we have commands left)
	return (_currentCommand >= 0 && _currentCommand < _commands.Num());
}

ConversationStatePtr Conversation::GetConversationState(int actor)
{
	idAI* ai = GetActor(actor);

	if (ai == NULL)
	{
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Conversation %s could not find actor.\r", _name.c_str());
		return ConversationStatePtr();
	}

	// Let's see if the AI can handle this conversation command
	ConversationStatePtr convState = std::dynamic_pointer_cast<ConversationState>(ai->GetMind()->GetState());

	if (convState == NULL)
	{
		// AI is not in ConversationState anymore
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Conversation %s: ai %s is not in conversation mode anymore.\r", _name.c_str(), ai->name.c_str());
		return ConversationStatePtr();
	}

	return convState;
}

idAI* Conversation::GetActor(int index)
{
	if (index < 0 || index >= _actors.Num()) 
	{
		return NULL; // index out of bounds
	}

	// Resolve the entity name and get the pointer
	idEntity* ent = gameLocal.FindEntity(_actors[index]);

	if (ent == NULL || !ent->IsType(idAI::Type)) 
	{
		// not an AI!
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Actor %s in conversation %s is not existing or of wrong type.\r", _actors[index].c_str(), _name.c_str());
		return NULL; 
	}

	return static_cast<idAI*>(ent);
}

idAI* Conversation::GetActor(const idStr& name)
{
	// greebo: Check if all actors are available
	for (int i = 0; i < _actors.Num(); i++)
	{
		if (_actors[i] == name) 
		{
			// index found, get the actor
			return GetActor(i);
		}
	}

	return NULL;
}

int Conversation::GetNumActors()
{
	return _actors.Num();
}

int Conversation::GetNumCommands()
{
	return _commands.Num();
}

void Conversation::Save(idSaveGame* savefile) const
{
	savefile->WriteString(_name);
	savefile->WriteBool(_isValid);
	savefile->WriteFloat(_talkDistance);

	savefile->WriteInt(_actors.Num());
	for (int i = 0; i < _actors.Num(); i++)
	{
		savefile->WriteString(_actors[i]);
	}

	savefile->WriteInt(_commands.Num());
	for (int i = 0; i < _commands.Num(); i++)
	{
		_commands[i]->Save(savefile);
	}

	savefile->WriteInt(_currentCommand);
	savefile->WriteInt(_playCount);
	savefile->WriteInt(_maxPlayCount);
	savefile->WriteBool(_actorsMustBeWithinTalkDistance);
	savefile->WriteBool(_actorsAlwaysFaceEachOtherWhileTalking);
}

void Conversation::Restore(idRestoreGame* savefile)
{
	savefile->ReadString(_name);
	savefile->ReadBool(_isValid);
	savefile->ReadFloat(_talkDistance);

	int num;
	savefile->ReadInt(num);
	_actors.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_actors[i]);
	}

	savefile->ReadInt(num);
	_commands.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		_commands[i] = ConversationCommandPtr(new ConversationCommand);
		_commands[i]->Restore(savefile);
	}

	savefile->ReadInt(_currentCommand);
	savefile->ReadInt(_playCount);
	savefile->ReadInt(_maxPlayCount);
	savefile->ReadBool(_actorsMustBeWithinTalkDistance);
	savefile->ReadBool(_actorsAlwaysFaceEachOtherWhileTalking);
}

void Conversation::InitFromSpawnArgs(const idDict& dict, int index)
{
	idStr prefix = va("conv_%d_", index);

	// A non-empty name is mandatory for a conversation
	if (!dict.GetString(prefix + "name", "", _name) || _name.IsEmpty())
	{
		// No conv_N_name spawnarg found, bail out
		_isValid = false;
		return;
	}

	// Parse "global" conversation settings 
	_talkDistance = dict.GetFloat(prefix + "talk_distance", "60");

	// Parse participant actors
	// Check if this entity can be used by others.
	idStr actorPrefix = prefix + "actor_";
	for (const idKeyValue* kv = dict.MatchPrefix(actorPrefix); kv != NULL; kv = dict.MatchPrefix(actorPrefix, kv))
	{
		// Add each actor name to the list
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Adding actor %s to conversation %s.\r", kv->GetValue().c_str(), _name.c_str());
		_actors.AddUnique(kv->GetValue());
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Conversation %s has %d actors.\r", _name.c_str(), _actors.Num());

	if (_actors.Num() == 0)
	{
		_isValid = false; // no actors, no conversation
		gameLocal.Warning("Ignoring conversation %s as it has no actors.", _name.c_str());
		return;
	}

	// Start parsing the conversation scripts (i.e. the commands), start with index 1
	for (int i = 1; i < INT_MAX; i++)
	{
		idStr cmdPrefix = va(prefix + "cmd_%d_", i);

		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Attempting to find command with index %d matching prefix %s.\r", i, cmdPrefix.c_str());

		if (dict.MatchPrefix(cmdPrefix) != NULL)
		{
			// Found a matching "conv_N_cmd_M..." spawnarg, start parsing 
			ConversationCommandPtr cmd(new ConversationCommand);

			// Let the command parse itself
			if (cmd->Parse(dict, cmdPrefix))
			{
				// Parsing succeeded, add this to the command list
				_commands.Append(cmd);
			}
		}
		else 
		{
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("No match found, terminating loop on index %d.\r", i);
			break;
		}
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("%d Commands found for Conversation %s.\r", _commands.Num(), _name.c_str());

	// Sanity check the commands
	if (_commands.Num() == 0) 
	{
		// No commands, what kind of conversation is this?
		_isValid = false;
		gameLocal.Warning("Ignoring conversation %s as it has no commands.", _name.c_str());
		return;
	}

	// Sanity check the talk distance
	if (_talkDistance <= 0.0f)
	{
		_isValid = false;
		gameLocal.Warning("Ignoring conversation %s as it has a talk distance <= 0.", _name.c_str());
		return;
	}

	// get max play count, default is -1, which means infinitely often
	_maxPlayCount = dict.GetInt(prefix + "max_play_count", "-1"); 

	// per default, the actors should be within talk distance before they start talking
	_actorsMustBeWithinTalkDistance = dict.GetBool(prefix + "actors_must_be_within_talkdistance", "1");

	_actorsAlwaysFaceEachOtherWhileTalking = dict.GetBool(prefix + "actors_always_face_each_other_while_talking", "1");

	// greebo: For conversations with one actor some flags don't make sense
	if (_actors.Num() == 1)
	{
		_actorsMustBeWithinTalkDistance = false;
		_actorsAlwaysFaceEachOtherWhileTalking = false;
	}

	// Seems like we have everything we need
	_isValid = true;
}

} // namespace ai
