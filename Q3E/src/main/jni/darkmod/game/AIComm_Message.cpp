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
/*
* Implementation of the CAIComm_Message class.
*
* Copyright 2006: Damon Hill (sophisticatedZombie), USA
*
*/


/*
*--------------------------------------------------------------------------
* Includes
*--------------------------------------------------------------------------
*/
#include "precompiled.h"
#pragma hdrstop



#include "DarkModGlobals.h"
#include "AIComm_Message.h"

namespace ai {

/*
*--------------------------------------------------------------------------
* Constructor and Destructor
*--------------------------------------------------------------------------
*/

CommMessage::CommMessage
(
	TCommType in_commType,
	idEntity* in_p_issuingEntity,
	idEntity* in_p_recipientEntity,
	idEntity* in_p_directObjectEntity,
	const idVec3& in_directObjectLocation,
	int in_eventID // grayman #3424
)
{
	// Set member variables
	m_commType = in_commType;
	m_p_issuingEntity = in_p_issuingEntity;
	m_p_recipientEntity = in_p_recipientEntity;
	m_p_directObjectEntity = in_p_directObjectEntity;
	m_directObjectLocation = in_directObjectLocation;
	m_eventID = in_eventID; // grayman #3424

	m_msgTag = 0; // grayman #3355

	// Record position of issuance
	m_positionOfIssuance.x = 0.0;
	m_positionOfIssuance.y = 0.0;
	m_positionOfIssuance.z = 0.0;
	if (m_p_issuingEntity.GetEntity() != NULL)
	{
		idPhysics* p_phys  = m_p_issuingEntity.GetEntity()->GetPhysics();
		if (p_phys != NULL)
		{
			m_positionOfIssuance = p_phys->GetOrigin();
		}
		else
		{
			DM_LOG (LC_STIM_RESPONSE, LT_WARNING)LOGSTRING("Issuer has no physics object, so stim considered to be issued from 0,0,0");
		}
	}
	else
	{
		DM_LOG (LC_STIM_RESPONSE, LT_WARNING)LOGSTRING("No issuing entity, so stim considered to be issued from 0,0,0");
	}

	// Done
}

CommMessage::CommMessage()
{
	m_commType = NumCommTypes;
	m_p_issuingEntity = NULL;
	m_p_recipientEntity = NULL;
	m_msgTag = 0; // grayman #3355
	m_p_directObjectEntity = NULL;
	m_eventID = -1; // grayman #3424
	m_directObjectLocation.Zero();
	m_positionOfIssuance.Zero();
}

void CommMessage::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(static_cast<int>(m_commType));
	m_p_issuingEntity.Save(savefile);
	m_p_recipientEntity.Save(savefile);
	m_p_directObjectEntity.Save(savefile);
	savefile->WriteVec3(m_directObjectLocation);
	savefile->WriteInt(m_eventID); // grayman #3424
	savefile->WriteVec3(m_positionOfIssuance);
	savefile->WriteInt(m_msgTag); // grayman #3355
}

void CommMessage::Restore(idRestoreGame *savefile)
{
	int tempInt;
	savefile->ReadInt(tempInt);
	m_commType = static_cast<TCommType>(tempInt);
	m_p_issuingEntity.Restore(savefile);
	m_p_recipientEntity.Restore(savefile);
	m_p_directObjectEntity.Restore(savefile);
	savefile->ReadVec3(m_directObjectLocation);
	savefile->ReadInt(m_eventID); // grayman #3424
	savefile->ReadVec3(m_positionOfIssuance);
	savefile->ReadInt(m_msgTag); // grayman #3355
}

} // namespace ai
