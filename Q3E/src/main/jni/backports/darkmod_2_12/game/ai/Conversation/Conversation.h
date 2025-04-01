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

#ifndef __AI_CONVERSATION_H__
#define __AI_CONVERSATION_H__

#include "precompiled.h"

#include "ConversationCommand.h"
#include "../States/ConversationState.h"

namespace ai {

/**
 * greebo: This class encapsulates a single conversation between
 * two or more characters in the game.
 */
class Conversation
{
private:
	// The name of this conversation
	idStr _name;

	// whether this conversation has errors or not
	bool _isValid;

	float _talkDistance;

	// All actors participating in this conversation
	idStringList _actors;

	// The list of commands this conversation consists of (this is the actual "script")
	idList<ConversationCommandPtr> _commands;

	// The current conversation command index
	int _currentCommand;

	// Counter to tell how often this conversation has been played
	int _playCount;

	// Specifies how often this conversation can be played (-1 == infinitely often)
	int _maxPlayCount;

	// TRUE if the actors are supposed to walk towards each other before starting to talk
	bool _actorsMustBeWithinTalkDistance;

	// TRUE if the actors always turn to each other while talking
	bool _actorsAlwaysFaceEachOtherWhileTalking;

public:
	Conversation();

	// Construct a conversation using the given spawnargs, using the given index
	Conversation(const idDict& spawnArgs, int index);

	/**
	 * greebo: Returns TRUE if this conversation is valid.
	 * Use this to check whether the construction of this class from given
	 * spawnargs has been successful.
	 */
	bool IsValid();

	// Returns the name of this conversation
	const idStr& GetName() const;

	// Returns the number of times this conversation has been (partially) played 
	int GetPlayCount();

	// Returns the maximum number of plays for this conversation
	int GetMaxPlayCount();

	// Returns true or false depending on the internal setting
	bool ActorsMustBeWithinTalkdistance();

	// Returns true or false depending on the internal setting
	bool ActorsAlwaysFaceEachOtherWhileTalking();

	// Returns the maximum distance actors can talk to each other from
	float GetTalkDistance();

	/**
	 * greebo: Returns TRUE if this conversation can be played. This basically means
	 * that all actors participating in this conversation are conscious.
	 */
	bool CheckConditions();

	/**
	 * greebo: Starts this conversation. Note that you should have called
	 * CheckConditions() beforehand (with a positive result, of course).
	 */
	void Start();

	/** 
	 * greebo: Ends this conversation and notifies the AI about this.
	 * Normally, the AI are put back into their previous state.
	 */
	void End();

	/**
	 * greebo: This is the "think" routine for conversations.
	 *
	 * @returns: TRUE if the process was successful, FALSE on error.
	 */
	bool Process();

	// Returns TRUE if the conversation has no more commands to execute
	bool IsDone();

	// Gets the actor with the given index/name
	idAI* GetActor(int index);
	idAI* GetActor(const idStr& name);

	// Returns the number of involved actors
	int GetNumActors();

	// Returns the number of commands in this conversation
	int GetNumCommands();

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	// Returns FALSE if any of the actors have gone dead or knocked out
	bool CheckActorAvailability();

	// Returns true if all actors are have execution state == "ready"
	bool AllActorsReady();

	// Returns the conversation state of the given actor (can be NULL)
	ConversationStatePtr GetConversationState(int actor);

	// Helper function to parse the properties from the spawnargs
	void InitFromSpawnArgs(const idDict& dict, int index);
};
typedef std::shared_ptr<Conversation> ConversationPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_H__ */
