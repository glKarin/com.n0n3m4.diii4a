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



#include "UserManager.h"

int UserManager::GetNumUsers()
{
	return m_users.Num();
}

void UserManager::AddUser(idActor* actor)
{
	idEntityPtr<idActor> actorPtr;
	actorPtr = actor;

	int index(m_users.FindIndex(actorPtr));
	if ( index < 0 ) {
		// angua: actor is not in users list yet
		if (actor->IsType(idAI::Type))
		{
			idAI* ai = static_cast<idAI*>(actor);
			int num = m_users.Num();
			idActor* currentActor;
			// go through users list
			for (int i = 0; i < num; i++)
			{
				currentActor = m_users[i].GetEntity();
				if (currentActor->IsType(idAI::Type))
				{
					idAI* currentAI = static_cast<idAI*>(currentActor);
					if (ai->AI_AlertLevel > currentAI->AI_AlertLevel)
					{
						// the ai's alert level is higher than the ai at this position in the list, add it in front of this
						m_users.Insert(actorPtr, i);
						return;
					}
				}
			}
			// otherwise, append at the end of the list
			m_users.Append(actorPtr);
		}
	}
}

// grayman #1327 - need to add a user w/o caring what the alert levels are

void UserManager::AppendUser(idActor* actor)
{
	idEntityPtr<idActor> actorPtr;
	actorPtr = actor;

	int index(m_users.FindIndex(actorPtr));
	if ( index < 0 )
	{
		// Actor is not in users list yet

		if (actor->IsType(idAI::Type))
		{
			m_users.Append(actorPtr); // Append at the end of the list
		}
	}
}

void UserManager::RemoveUser(idActor* actor)
{
	idEntityPtr<idActor> actorPtr;
	actorPtr = actor;

	int index = m_users.FindIndex(actorPtr);
	if (index >= 0)
	{
		m_users.RemoveIndex(index);
	}
}

idActor* UserManager::GetMasterUser()
{
	if (GetNumUsers() > 0)
	{
		return m_users[0].GetEntity();
	}
	return NULL;
}


idActor* UserManager::GetUserAtIndex(const int index) // grayman #2345
{
	if (GetNumUsers() > index)
	{
		return m_users[index].GetEntity();
	}
	return NULL;
}

void UserManager::InsertUserAtIndex(idActor* actor,const int index) // grayman #2345
{
	if ((actor != NULL) && (index >= 0))
	{
		idEntityPtr<idActor> actorPtr;
		actorPtr = actor;
		m_users.Insert(actorPtr,index);
	}
}

int UserManager::GetIndex(idActor* actor) // grayman #2345
{
	idEntityPtr<idActor> actorPtr;
	actorPtr = actor;
	return (m_users.FindIndex(actorPtr));
}

// grayman #2345/#2706 - whoever's closest to the door now gets to be master,
// so he has to be moved to the top of the user list. This cuts down on
// confusion around doors.

// TODO: Runners might need to go before walkers, regardless of distance from door.
// TODO: The rank of the AI might need to be considered, with nobles preceeding beggars.

void UserManager::ResetMaster(CFrobDoor* frobDoor)
{
	int numUsers = GetNumUsers();
	if (numUsers > 1)
	{
		// grayman #3755 - the first pass is to establish order based on proximity to the door
		idVec3 doorCenter = frobDoor->GetClosedBox().GetCenter(); // grayman #3643 - use center of door, not origin
		//idVec3 doorOrigin = frobDoor->GetPhysics()->GetOrigin();
		idActor* closestUser = NULL;	// the user closest to the door
		int masterIndex = 0;			// index of closest user
		float minDistance = 100000;		// minimum distance of all users
		for (int i = 0 ; i < numUsers ; i++)
		{
			idActor* user = frobDoor->GetUserManager().GetUserAtIndex(i);
			if (user != NULL)
			{
				float distance = (user->GetPhysics()->GetOrigin() - doorCenter).LengthFast();
				if (distance < minDistance)
				{
					masterIndex = i;
					closestUser = user;
					minDistance = distance;
				}
			}
		}

		if (masterIndex > 0) // only rearrange the queue if someone other than the current master is closer
		{
			RemoveUser(closestUser);			// remove AI from current spot
			InsertUserAtIndex(closestUser,0);	// and put him at the top
		}

		// grayman #3755 - the second pass is to establish order based on who's running.
		idActor* runner = NULL;	// the user running to the door
		masterIndex = 0;		// index of running user
		for (int i = 0 ; i < numUsers ; i++)
		{
			idActor* user = frobDoor->GetUserManager().GetUserAtIndex(i);
			if (user != NULL)
			{
				if (static_cast<idAI*>(user)->AI_RUN)
				{
					masterIndex = i;
					runner = user;
					break;
				}
			}
		}

		if (masterIndex > 0) // only rearrange the queue if someone other than the current master is running
		{
			RemoveUser(runner);			 // remove AI from current spot
			InsertUserAtIndex(runner,0); // and put him at the top
		}
	}
}


void UserManager::Save(idSaveGame* savefile) const
{
	int num = m_users.Num();
	savefile->WriteInt(num);
	for (int i = 0; i < num; i++)
	{
		m_users[i].Save(savefile);
	}
}

void UserManager::Restore(idRestoreGame* savefile)
{
	int num;
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		idEntityPtr<idActor> actor;
		actor.Restore(savefile);
		m_users.AddUnique(actor);
	}
}

