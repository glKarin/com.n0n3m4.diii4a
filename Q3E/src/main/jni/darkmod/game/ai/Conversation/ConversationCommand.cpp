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
#include "ConversationCommand.h"
#include "../States/ConversationState.h"

namespace ai {

// These are the various command type strings, needed for parsing
const char* const ConversationCommand::TypeNames[ConversationCommand::ENumCommands] =
{
	"WaitSeconds",
	"WalkToActor",
	"WalkToPosition",
	"WalkToEntity",
	"StopMove",
	"Talk",
	"PlayAnimOnce",
	"PlayAnimCycle",
	"ActivateTarget",
	"LookAtActor",
	"LookAtPosition",
	"LookAtEntity",
	"TurnToActor",
	"TurnToPosition",
	"TurnToEntity",
	"AttackActor",
	"AttackEntity",
	"InteractWithEntity",
	"RunScript",
	"WaitForActor",
	"WaitForAllActors",
};

ConversationCommand::ConversationCommand() :
	_type(ENumCommands), // invalid type
	_state(EReadyForExecution)
{}

ConversationCommand::Type ConversationCommand::GetType()
{
	return _type;
}

ConversationCommand::State ConversationCommand::GetState()
{
	return _state;
}

bool ConversationCommand::WaitUntilFinished()
{
	return _waitUntilFinished;
}

void ConversationCommand::SetState(ConversationCommand::State newState)
{
	_state = newState;
}

int ConversationCommand::GetActor()
{
	return _actor;
}

int ConversationCommand::GetNumArguments()
{
	return _arguments.Num();
}

// Returns the given argument (starting with index 0) or "" if the argument doesn't exist
idStr ConversationCommand::GetArgument(int index)
{
	return (index >= 0 && index < _arguments.Num()) ? _arguments[index] : "";
}

idEntity* ConversationCommand::GetEntityArgument(int index)
{
	return gameLocal.FindEntity(GetArgument(index));
}

idVec3 ConversationCommand::GetVectorArgument(int index)
{
	idVec3 returnValue(0,0,0);
	sscanf( GetArgument(index), "%f %f %f", &returnValue.x, &returnValue.y, &returnValue.z );
	return returnValue;
}

float ConversationCommand::GetFloatArgument(int index)
{
	return atof(GetArgument(index));
}

bool ConversationCommand::Parse(const idDict& dict, const idStr& prefix)
{
	// Get the type
	_type = GetType(dict.GetString(prefix + "type", idStr(ENumCommands)));

	if (_type == ENumCommands)
	{
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Could not find type for command prefix %s.\r", prefix.c_str());
		return false; // invalid command type
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Found command type %s for prefix %s.\r", TypeNames[_type], prefix.c_str());

	// Parse the executing actor
	_actor = dict.GetInt(prefix + "actor", "0");

	if (_actor <= 0)
	{
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Conversation command %s: Invalid actor index %d encountered.\r", TypeNames[_type], _actor);
		return false;
	}
	
	// Decrease the actor index, so that it can be used as index in the actors idList.
	_actor--;

	// Check if we have a "wait_until_finished" argument (default is "1")
	_waitUntilFinished = dict.GetBool(prefix + "wait_until_finished", "1");

	// Parse the arguments
	_arguments.Clear();

	idStr argPrefix = prefix + "arg_";
	for (int i = 1; i < INT_MAX; i++)
	{
		idStr arg = dict.GetString(argPrefix + idStr(i));

		if (arg.IsEmpty()) break;

		// Add each argument to the list
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Adding argument %s to conversation command %s.\r", arg.c_str(), TypeNames[_type]);
		_arguments.Append(arg);
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Command type %s has %d arguments.\r", TypeNames[_type], _arguments.Num());

	return true;
}

void ConversationCommand::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(static_cast<int>(_type));
	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_actor);
	savefile->WriteBool(_waitUntilFinished);

	savefile->WriteInt(_arguments.Num());
	for (int i = 0; i < _arguments.Num(); i++)
	{
		savefile->WriteString(_arguments[i]);
	}
}

void ConversationCommand::Restore(idRestoreGame* savefile)
{
	int temp;
	savefile->ReadInt(temp);
	assert(temp >= 0 && temp <= ENumCommands); // sanity check
	_type = static_cast<Type>(temp);

	savefile->ReadInt(temp);
	assert(temp >= 0 && temp <= ENumStates); // sanity check
	_state = static_cast<State>(temp);

	savefile->ReadInt(_actor);
	savefile->ReadBool(_waitUntilFinished);

	int num;
	savefile->ReadInt(num);
	_arguments.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_arguments[i]);
	}
}

ConversationCommand::Type ConversationCommand::GetType(const idStr& cmdString)
{
	for (int i = 0; i < ENumCommands; i++)
	{
		if (cmdString == TypeNames[i])
		{
			return static_cast<Type>(i);
		}
	}

	return ENumCommands;
}

} // namespace ai
