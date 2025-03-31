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



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "FrobHandle.h"

//===============================================================================
// CFrobHandle
//===============================================================================
const idEventDef EV_TDM_Handle_Tap( "Tap", EventArgs(), EV_RETURNS_VOID, "Operates this handle." );

CLASS_DECLARATION( CBinaryFrobMover, CFrobHandle )
	EVENT( EV_TDM_Handle_Tap,	CFrobHandle::Event_Tap )
END_CLASS

CFrobHandle::CFrobHandle() :
	m_FrobMaster(NULL),
	m_IsMasterHandle(true),
	m_FrobLock(false)
{}

void CFrobHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_FrobMaster);
	savefile->WriteBool(m_IsMasterHandle);
	savefile->WriteBool(m_FrobLock);
}

void CFrobHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_FrobMaster));
	savefile->ReadBool(m_IsMasterHandle);
	savefile->ReadBool(m_FrobLock);
}

void CFrobHandle::Spawn()
{
	// Dorhandles are always non-interruptable
	m_bInterruptable = false;

	// greebo: The handle itself must never locked, otherwise it can't move in Tap()
	m_Lock->SetLocked(false);
}

void CFrobHandle::Event_Tap()
{
	Tap();
}

void CFrobHandle::SetFrobbed(const bool val)
{
	if (m_FrobLock) return; // Prevent an infinite loop here.

	m_FrobLock = true;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("CFrobHandle [%s] %08lX is frobbed\r", name.c_str(), this);

	idEntity::SetFrobbed(val);

	if (m_FrobMaster != NULL)
	{
		m_FrobMaster->SetFrobbed(val);
	}

	m_FrobLock = false;
}

bool CFrobHandle::IsFrobbed() const
{
	return (m_FrobMaster != NULL) ? m_FrobMaster->IsFrobbed() : idEntity::IsFrobbed();
}

void CFrobHandle::SetFrobMaster(idEntity* frobMaster)
{
	m_FrobMaster = frobMaster;
}

idEntity* CFrobHandle::GetFrobMaster()
{
	return m_FrobMaster;
}

bool CFrobHandle::CanBeUsedByItem(const CInventoryItemPtr& item, bool isFrobUse)
{
	// Pass the call to the master, if we have one, otherwise let the base class handle it
	return (m_FrobMaster != NULL) ? m_FrobMaster->CanBeUsedByItem(item, isFrobUse) : idEntity::CanBeUsedByItem(item, isFrobUse);
}

bool CFrobHandle::UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	// Pass the call to the master, if we have one, otherwise let the base class handle it
	return (m_FrobMaster != NULL) ? m_FrobMaster->UseByItem(impulseState, item) : idEntity::UseByItem(impulseState, item);
}

void CFrobHandle::AttackAction(idPlayer* player)
{
	if (m_FrobMaster != NULL)
	{
		m_FrobMaster->AttackAction(player);
	}
}

/*void CFrobHandle::FrobAction(bool bMaster)
{
	if (m_FrobMaster != NULL)
	{
		m_FrobMaster->FrobAction(bMaster);
	}
}*/

void CFrobHandle::ToggleLock() 
{}

bool CFrobHandle::IsMasterHandle()
{
	return m_IsMasterHandle;
}

void CFrobHandle::SetMasterHandle(bool isMaster)
{
	m_IsMasterHandle = isMaster;
}

void CFrobHandle::Tap()
{
	// Default action: Trigger the handle movement
	ToggleOpen();
}

bool CFrobHandle::GetPhysicsToSoundTransform(idVec3 &origin, idMat3 &axis)
{
	idVec3 eyePos = gameLocal.GetLocalPlayer()->GetEyePosition();
	const idBounds& bounds = GetPhysics()->GetAbsBounds();

	//gameRenderWorld->DebugBounds(colorLtGrey, bounds, vec3_origin, 5000);
	
	// greebo: Choose the corner which is nearest to the player's eyeposition
	origin.x = (idMath::Fabs(bounds[0].x - eyePos.x) < idMath::Fabs(bounds[1].x - eyePos.x)) ? bounds[0].x : bounds[1].x;
	origin.y = (idMath::Fabs(bounds[0].y - eyePos.y) < idMath::Fabs(bounds[1].y - eyePos.y)) ? bounds[0].y : bounds[1].y;
	origin.z = (idMath::Fabs(bounds[0].z - eyePos.z) < idMath::Fabs(bounds[1].z - eyePos.z)) ? bounds[0].z : bounds[1].z;

	// The called expects the origin in local space
	origin -= GetPhysics()->GetOrigin();

	axis.Identity();

	//gameRenderWorld->DebugArrow(colorWhite, GetPhysics()->GetOrigin() + origin, eyePos, 0, 5000);

	return true;
}
