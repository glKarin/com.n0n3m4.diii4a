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

#ifndef __AI_CONVERSATION_STATE_H__
#define __AI_CONVERSATION_STATE_H__

#include "State.h"
#include "../Conversation/ConversationCommand.h"

namespace ai
{

#define STATE_CONVERSATION "Conversation"

class ConversationState :
	public State
{
public:
	enum ExecutionState
	{
		ENotReady = 0,		// not ready yet (try next frame)
		EReady,				// ready for starting
		EExecuting,			// executing, but ready for new commands
		EBusy,				// execution in progress, can't handle new commands
		ENumExecutionStates,// invalid index
	};

private:
	// The conversation index
	int _conversation;

	// The execution state
	ExecutionState _state;

	// The conversation command type
	ConversationCommand::Type _commandType;

	int _finishTime;

	ConversationState();

public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	// Incoming events issued by the Subsystems
	virtual void OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem) override;

	// Sets the conversation this state should handle
	void SetConversation(int index);

	/**
	 * greebo: Processes the given command belonging to the given conversation.
	 * The command's state variable is updated after this call and corresponds 
	 * to the execution state of this AI.
	 */
	void ProcessCommand(ConversationCommand& command);

	// Starts execution of the given command, returns FALSE on failure
	void StartCommand(ConversationCommand& command, Conversation& conversation);

	// Handles the given command, returns FALSE on failure
	void Execute(ConversationCommand& command, Conversation& conversation);

	// Returns the current conversation command execution state
	ConversationState::ExecutionState GetExecutionState();

	// This is called when a State is destroyed
	virtual void Cleanup(idAI* owner) override; // grayman #3559

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner) override;

	static StatePtr CreateInstance();

private:
	// Plays the given sound (shader) and returns the length in msecs
	int Talk(idAI* owner, const idStr& soundName);

	// Returns true if the conversation can be started
	bool CheckConversationPrerequisites();

	// Returns the execution state of the given actor of the given conversation
	ExecutionState GetConversationStateOfActor(idAI* ai);

	// Private helper for debug output
	void DrawDebugOutput(idAI* owner);

	virtual void OnActorEncounter(idEntity* stimSource, idAI* owner) override;

	void Wrapup(idAI* owner);
};
typedef std::shared_ptr<ConversationState> ConversationStatePtr;

} // namespace ai

#endif /* __AI_CONVERSATION_STATE_H__ */
