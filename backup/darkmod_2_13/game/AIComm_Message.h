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
/******************************************************************************/
/*                                                                            */
/*         CommunicationStim									              */
/*         Copyright 2006 Damon Hill (sophisticatedZombie)  2006              */
/*																			  */
/******************************************************************************/

/*
*--------------------------------------------------------------------------
* Header wrapper
*--------------------------------------------------------------------------
*/
#ifndef COMMUNICATION_MESSAGE_HEADER
#define COMMUNICATION_MESSAGE_HEADER

#include "StimResponse/StimResponse.h"

namespace ai {

/**
 * This struct carries information which is used by AI 
 * to communicate with each other.
 */
struct CommMessage
{
	/*!
	* This enumeration defines the meaning of the communication.
	* Each enumeration value indicates 
	*/
	enum TCommType
	{
		/*!
		* A friendly greeting, such as "hello"
		*
		* recipientEntity: The entity being greeted
		*/
		Greeting_CommType = 0x01,

		/*!
		* A friendly joke not meant to insult anyone.
		*
		* recipientEntity: The entity meant to hear it
		* directObjectEntity; The butt of the joke
		*/
		FriendlyJoke_CommType,

		/*!
		* An insult directed at some entity.
		* "<X>'s mother was a hamster, and his father smelt of elderberries!"
		*
		* recipientEntity: The entity meant to hear it
		* directObjectEntity: The entity that is the subject of the insult
		*/
		Insult_CommType,

		/*!
		* A request for help of any kind at issuing entity.
		*
		* directObjectEntity: The entity that caused the need for help
		* directObjectLocation: The location of the enemy when the
		*	request was issued
		*/
		RequestForHelp_CommType,

		/*!
		* A request for long range combat help at issuing entity.
		*
		* directObjectEntity: The entity that caused the need for help
		* directObjectLocation: The location of the enemy when the
		*	request was issued
		*/
		RequestForMissileHelp_CommType,

		/*!
		* A request for close range combat help at issuing entity.
		*
		* directObjectEntity: The entity that caused the need for help
		* directObjectLocation: The location of the enemy when the
		*	request was issued
		*/
		RequestForMeleeHelp_CommType,

		/*!
		* A request for someone to bring a light to a given location.
		*
		* directObjectLocation: The location where the light
		*	is needed
		*/
		RequestForLight_CommType,

		/*!
		* A general declaration that something suspicious has been 
		* detected.
		*
		* directObjectEntity: If not NULL, the entity that is suspicious
		* directObjectLocation: The location of the detected suspicious
		*	stimulus
		*/
		DetectedSomethingSuspicious_CommType,

		/*!
		* A general declaration that an enemy has been detected.
		*
		* directObjectEntity: The enemy that was detected
		* directObjectLocation: The location of the enemy at the time
		*	of detection
		*/
		DetectedEnemy_CommType,

		/*!
		* An order to follow the issuer.
		*/
		FollowOrder_CommType,	

		/*!
		* An order to guard a location.
		* 
		* directObjectLocation: The location to be guarded
		*/
		GuardLocationOrder_CommType,

		/*!
		* An order to guard an entity
		*
		* directObjectEntity: The entity to be guarded
		*/
		GuardEntityOrder_CommType,

		/*!
		* An order to perform a certain patrol
		*
		* ???
		*/
		PatrolOrder_CommType,

		/*
		* An order to attack a certain entity
		*
		* directObjectEntity: The entity to be attacked
		*/
		AttackOrder_CommType,

		/*
		* This is sent from an AI to another AI that is
		* in the way of its movement goal.
		*
		* The proper response is to have a uniform way of
		* passing each other
		*/
		GetOutOfTheWayOrder_CommType,

		/**
		 * greebo: These three were defined in the script, but not here >> added.
		 */
		ConveyWarning_EvidenceOfIntruders_CommType,
		ConveyWarning_ItemsHaveBeenStolen_CommType,
		ConveyWarning_EnemiesHaveBeenSeen_CommType,

		ConveyWarning_CorpseHasBeenSeen_CommType, // grayman #1327
		ConveyWarning_UnconsciousPersonHasBeenSeen_CommType, // grayman #3857

		// grayman #3857 - initial discoveries must be responded to
		RaiseTheAlarm_ItemsHaveBeenStolen_CommType,
		RaiseTheAlarm_CorpseHasBeenSeen_CommType,
		RaiseTheAlarm_UnconsciousPersonHasBeenSeen_CommType,

		NumCommTypes, // invalid type
	};

	// Default constructor, only used for saving/restoring
	CommMessage();

	/*!
	* public Constructor
	*/
	CommMessage(
		TCommType in_commType,
		idEntity* in_p_issuingEntity,
		idEntity* in_p_recipientEntity,
		idEntity* in_p_directObjectEntity,
		const idVec3& in_directObjectLocation,
		int in_eventID // grayman #3424
	);

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	/*!
	* This field indicates the type of communication. The value of this type defines
	* the meaning of the other member variables. (See typedef of TCommType for specs)
	*/
	TCommType m_commType;

	/*!
	* This field indicates the entity which issued the communication
	*/
	idEntityPtr<idEntity> m_p_issuingEntity;

	/*!
	* This field indicates the entity which is the intended recipient of the
	* communication.  A value of NULL indicates the communication has no
	* specific recipient in mind.
	*/
	idEntityPtr<idEntity> m_p_recipientEntity;

	/*!
	* This field contains a unique number that associates this message with
	* a particular propagated sound.
	*/
	int m_msgTag; // grayman #3355

	/*!
	* This field indicates the entity which is the entity mentioned in the
	* communication.
	*/
	idEntityPtr<idEntity> m_p_directObjectEntity;

	/*!
	* This field indicates the location which is the location mentioned in
	* the communication.
	*/
	idVec3 m_directObjectLocation;

	/*
	* grayman #3424
	*/
	int m_eventID;

	/*!
	* This is the position of the issuing entity at the time of issuance
	*/
	idVec3 m_positionOfIssuance;
};
typedef std::shared_ptr<CommMessage> CommMessagePtr;
typedef idList<ai::CommMessagePtr> MessageList;

} // namespace ai

#endif
