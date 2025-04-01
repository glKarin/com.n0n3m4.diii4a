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

#ifndef __AI_CONVERSATION_SYSTEM_H__
#define __AI_CONVERSATION_SYSTEM_H__

#include "precompiled.h"

#include "Conversation.h"

namespace ai {

class ConversationSystem
{
private:
	// The indexed list of conversations
	idList<ConversationPtr> _conversations;

	// The indices of all active conversations
	idList<int> _activeConversations;

	// The indices of all finished conversations, ready for removal
	idList<int> _dyingConversations;

public:
	// Clears and removes all allocated data
	void Clear();

	/**
	 * greebo: Initialises this class. This means loading the conversation entities
	 * containing all conversations for this map.
	 */
	void Init(idMapFile* mapFile);

	/**
	 * greebo: Returns the conversation for the given name/index or NULL if not found.
	 */
	ConversationPtr GetConversation(const idStr& name);
	ConversationPtr GetConversation(int index);

	/**
	 * Returns the numeric index for the given conversation name or -1 if not found.
	 */
	int GetConversationIndex(const idStr& name);

	/**
	 * greebo: Returns the number of available conversations in this system.
	 */
	int GetNumConversations();

	/**
	 * greebo: This starts the conversation, after checking whether all conditions are met.
	 *
	 * @index: The conversation index. Use GetConversationIndex() to convert a conversation name to an index.
	 */
	void StartConversation(int index);

	/**
	 * greebo: Stops the given conversation. This can be called during ProcessConversations() too,
	 * as the index is only marked for removal next frame.
	 */
	void EndConversation(int index);

	/**
	 * greebo: This is the "thinking" routine for conversations which
	 * remotely controls the participating actors.
	 */
	void ProcessConversations();

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	// Helper to load a conversation from an entity's spawnargs
	void LoadConversationEntity(idMapEntity* entity);
};
typedef std::shared_ptr<ConversationSystem> ConversationSystemPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_SYSTEM_H__ */
