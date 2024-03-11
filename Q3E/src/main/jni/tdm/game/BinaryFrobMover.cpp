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

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "ai/AAS_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "SndProp.h"
#include "StimResponse/StimResponse.h"

//===============================================================================
// CBinaryFrobMover
//===============================================================================

const idEventDef EV_TDM_FrobMover_Open( "Open", EventArgs(), EV_RETURNS_VOID, "Opens the frobmover, regardless of its previous state. The mover will not move when it's locked. ");
const idEventDef EV_TDM_FrobMover_Close( "Close", EventArgs(), EV_RETURNS_VOID, "Closes the frobmover, regardless of its previous state. Mover must be open, otherwise nothing happens.");
const idEventDef EV_TDM_FrobMover_ToggleOpen( "ToggleOpen", EventArgs(), EV_RETURNS_VOID, 
	"Toggles the mover state. Closes when fully open, opens when fully closed. If the mover is \"interrupted\" (e.g. when the player frobbed the mover in between), the move direction depends on the state of the internal \"intent_open\" flag. ");
const idEventDef EV_TDM_FrobMover_Lock( "Lock", EventArgs(), EV_RETURNS_VOID, "Locks the mover. Calls to Open() will not succeed after this call. ");
const idEventDef EV_TDM_FrobMover_Unlock( "Unlock", EventArgs(), EV_RETURNS_VOID, "Unlocks the mover. Calls to Open() will succeed after this call. Depending on the value of the spawnarg \"open_on_unlock\" the mover might automatically open after this call. ");
const idEventDef EV_TDM_FrobMover_ToggleLock( "ToggleLock", EventArgs(), EV_RETURNS_VOID, 
	"Toggles the lock state. Unlocked movers will be locked and vice versa.\n" \
	"The notes above concerning Unlock() still apply if this call unlocks the mover. ");
const idEventDef EV_TDM_FrobMover_IsOpen( "IsOpen", EventArgs(), 'f', 
	"Returns true (nonzero) if the mover is open, which is basically\n" \
	"the same as \"not closed\". A mover is considered closed when it is at its close position." );
const idEventDef EV_TDM_FrobMover_GetFractionalPosition( "GetFractionalPosition", EventArgs(), 'f', "Returns a fraction between 0.00 (closed) and 1.00 (open)." ); // grayman #4433
const idEventDef EV_TDM_FrobMover_IsLocked( "IsLocked", EventArgs(), 'f', "Returns true (nonzero) if the mover is currently locked." );
const idEventDef EV_TDM_FrobMover_IsPickable( "IsPickable", EventArgs(), 'f', "Returns true (nonzero) if this frobmover is pickable." );
const idEventDef EV_TDM_FrobMover_HandleLockRequest( "_handleLockRequest", EventArgs(), EV_RETURNS_VOID, "internal"); // used for periodic checks to lock the door once it is fully closed
const idEventDef EV_TDM_FrobMover_ClearPlayerImmobilization("_EV_TDM_FrobMover_ClearPlayerImmobilization", 
	EventArgs('e', "", ""), EV_RETURNS_VOID, "internal"); // grayman #3643 - allows player to handle weapons again


CLASS_DECLARATION( idMover, CBinaryFrobMover )
	EVENT( EV_PostSpawn,					CBinaryFrobMover::Event_PostSpawn )
	EVENT( EV_TDM_FrobMover_Open,			CBinaryFrobMover::Event_Open)
	EVENT( EV_TDM_FrobMover_Close,			CBinaryFrobMover::Event_Close)
	EVENT( EV_TDM_FrobMover_ToggleOpen,		CBinaryFrobMover::Event_ToggleOpen)
	EVENT( EV_TDM_FrobMover_Lock,			CBinaryFrobMover::Event_Lock)
	EVENT( EV_TDM_FrobMover_Unlock,			CBinaryFrobMover::Event_Unlock)
	EVENT( EV_TDM_FrobMover_ToggleLock,		CBinaryFrobMover::Event_ToggleLock)
	EVENT( EV_TDM_FrobMover_IsOpen,			CBinaryFrobMover::Event_IsOpen)
	EVENT( EV_TDM_FrobMover_IsLocked,		CBinaryFrobMover::Event_IsLocked)
	EVENT( EV_TDM_FrobMover_IsPickable,		CBinaryFrobMover::Event_IsPickable)
	EVENT( EV_Activate,						CBinaryFrobMover::Event_Activate)
	EVENT( EV_TDM_FrobMover_HandleLockRequest,	CBinaryFrobMover::Event_HandleLockRequest)
	EVENT( EV_TDM_FrobMover_ClearPlayerImmobilization,	CBinaryFrobMover::Event_ClearPlayerImmobilization ) // grayman #3643
	EVENT( EV_TDM_Lock_StatusUpdate,		CBinaryFrobMover::Event_Lock_StatusUpdate) // grayman #3643
	EVENT( EV_TDM_Lock_OnLockPicked,		CBinaryFrobMover::Event_Lock_OnLockPicked) // grayman #3643
	EVENT( EV_TDM_FrobMover_GetFractionalPosition,  CBinaryFrobMover::Event_GetFractionalPosition) // grayman #4433
END_CLASS

CBinaryFrobMover::CBinaryFrobMover()
{
	m_Lock = NULL;
	
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_binary_mover";
	m_Open = false;
	m_bInterruptable = true;
	m_bInterrupted = false;
	m_StoppedDueToBlock = false;
	m_nextBounceTime = 0; // grayman #3755
	m_LastBlockingEnt = NULL;
	m_bIntentOpen = false;
	m_StateChange = false;
	m_Rotating = false;
	m_Translating = false;
	m_StartPos = vec3_zero;
	m_Translation = vec3_zero;
	m_TransSpeed = 0;
	m_ImpulseThreshCloseSq = 0;
	m_ImpulseThreshOpenSq = 0;
	m_vImpulseDirOpen.Zero();
	m_vImpulseDirClose.Zero();
	m_stopWhenBlocked = false;
	m_LockOnClose = false;
	m_mousePosition.Zero();
	m_bFineControlStarting = false;
	m_closedBox = box_zero; // grayman #2345 - holds closed position
	m_closedBox.Clear();	// grayman #2345
	m_registeredAI.Clear();	// grayman #1145
	m_lastUsedBy = NULL;	// grayman #2859
	m_searching = NULL;		// grayman #1327 - someone searching around this door
	m_targetingOff = false; // grayman #3029
	m_wasFoundLocked = false; // grayman #3104
	m_timeDoorStartedMoving = 0; // grayman #3462
	m_ClosedOrigin = vec3_zero;
	m_OpenOrigin = vec3_zero;
	m_ClosedPos = vec3_zero;
	m_OpenPos = vec3_zero;
	m_OpenDir = vec3_zero;
}

CBinaryFrobMover::~CBinaryFrobMover()
{
	delete m_Lock;
}

idBox CBinaryFrobMover::GetClosedBox() // grayman #2345
{
	return m_closedBox;
}

void CBinaryFrobMover::AddObjectsToSaveGame(idSaveGame* savefile)
{
	idEntity::AddObjectsToSaveGame(savefile);

	savefile->AddObject(m_Lock);
}

void CBinaryFrobMover::Save(idSaveGame *savefile) const
{
	// The lock class is saved by the idSaveGame class on close, no need to handle it here
	savefile->WriteObject(m_Lock);

	savefile->WriteBool(m_Open);
	savefile->WriteBool(m_bIntentOpen);
	savefile->WriteBool(m_StateChange);
	savefile->WriteBool(m_bInterruptable);
	savefile->WriteBool(m_bInterrupted);
	savefile->WriteBool(m_StoppedDueToBlock);
	savefile->WriteInt(m_nextBounceTime); // grayman #3755

	m_LastBlockingEnt.Save(savefile);
	
	savefile->WriteAngles(m_Rotate);
		
	savefile->WriteVec3(m_StartPos);
	savefile->WriteVec3(m_Translation);
	savefile->WriteFloat(m_TransSpeed);

	savefile->WriteAngles(m_ClosedAngles);
	savefile->WriteAngles(m_OpenAngles);

	savefile->WriteVec3(m_ClosedOrigin);
	savefile->WriteVec3(m_OpenOrigin);

	savefile->WriteVec3(m_ClosedPos);
	savefile->WriteVec3(m_OpenPos);
	savefile->WriteVec3(m_OpenDir);

	savefile->WriteString(m_CompletionScript);

	savefile->WriteBool(m_Rotating);
	savefile->WriteBool(m_Translating);
	savefile->WriteFloat(m_ImpulseThreshCloseSq);
	savefile->WriteFloat(m_ImpulseThreshOpenSq);
	savefile->WriteVec3(m_vImpulseDirOpen);
	savefile->WriteVec3(m_vImpulseDirClose);

	savefile->WriteBool(m_stopWhenBlocked);
	savefile->WriteBool(m_LockOnClose);
	savefile->WriteBool(m_bFineControlStarting);
	savefile->WriteBox(m_closedBox); // grayman #2345
	
	// grayman #1145 - registered AI for a locked door
	savefile->WriteInt(m_registeredAI.Num());
	for (int i = 0 ; i < m_registeredAI.Num() ; i++ )
	{
		m_registeredAI[i].Save(savefile);
	}

	m_lastUsedBy.Save(savefile);			// grayman #2859
	m_searching.Save(savefile);				// grayman #1327
	savefile->WriteBool(m_targetingOff);	// grayman #3029
	savefile->WriteBool(m_wasFoundLocked);	// grayman #3104
	savefile->WriteInt(m_timeDoorStartedMoving); // grayman #3462
}

void CBinaryFrobMover::Restore( idRestoreGame *savefile )
{
	// The lock class is restored by the idRestoreGame, don't handle it here
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Lock));

	savefile->ReadBool(m_Open);
	savefile->ReadBool(m_bIntentOpen);
	savefile->ReadBool(m_StateChange);
	savefile->ReadBool(m_bInterruptable);
	savefile->ReadBool(m_bInterrupted);
	savefile->ReadBool(m_StoppedDueToBlock);
	savefile->ReadInt(m_nextBounceTime); // grayman #3755

	m_LastBlockingEnt.Restore(savefile);
	
	savefile->ReadAngles(m_Rotate);
	
	savefile->ReadVec3(m_StartPos);
	savefile->ReadVec3(m_Translation);
	savefile->ReadFloat(m_TransSpeed);

	savefile->ReadAngles(m_ClosedAngles);
	savefile->ReadAngles(m_OpenAngles);

	savefile->ReadVec3(m_ClosedOrigin);
	savefile->ReadVec3(m_OpenOrigin);

	savefile->ReadVec3(m_ClosedPos);
	savefile->ReadVec3(m_OpenPos);
	savefile->ReadVec3(m_OpenDir);

	savefile->ReadString(m_CompletionScript);

	savefile->ReadBool(m_Rotating);
	savefile->ReadBool(m_Translating);
	savefile->ReadFloat(m_ImpulseThreshCloseSq);
	savefile->ReadFloat(m_ImpulseThreshOpenSq);
	savefile->ReadVec3(m_vImpulseDirOpen);
	savefile->ReadVec3(m_vImpulseDirClose);

	savefile->ReadBool(m_stopWhenBlocked);
	savefile->ReadBool(m_LockOnClose);
	savefile->ReadBool(m_bFineControlStarting);
	savefile->ReadBox(m_closedBox); // grayman #2345

	// grayman #1145 - registered AI for a locked door
	m_registeredAI.Clear();
	int num;
	savefile->ReadInt(num);
	m_registeredAI.SetNum(num);
	for (int i = 0 ; i < num ; i++)
	{
		m_registeredAI[i].Restore(savefile);
	}

	m_lastUsedBy.Restore(savefile);				// grayman #2859
	m_searching.Restore(savefile);				// grayman #1327
	savefile->ReadBool(m_targetingOff);			// grayman #3029
	savefile->ReadBool(m_wasFoundLocked);		// grayman #3104
	savefile->ReadInt(m_timeDoorStartedMoving); // grayman #3462
}

void CBinaryFrobMover::Spawn()
{
	// Setup our PickableLock instance
	m_Lock = static_cast<PickableLock*>(PickableLock::CreateInstance());
	m_Lock->SetOwner(this);
	m_Lock->SetLocked(false);

	m_stopWhenBlocked = spawnArgs.GetBool("stop_when_blocked", "1");

	m_Open = spawnArgs.GetBool("open");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] open (%u)\r", name.c_str(), m_Open);

	m_bInterruptable = spawnArgs.GetBool("interruptable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] interruptable (%u)\r", name.c_str(), m_bInterruptable);

	// log if visportal was found
	if( areaPortal > 0 )
	{
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("FrobDoor [%s] found portal handle %d on spawn \r", name.c_str(), areaPortal);
	}

	// Load the spawnargs for the lock
	m_Lock->InitFromSpawnargs(spawnArgs);
	
	// Schedule a post-spawn event to parse the rest of the spawnargs
	// greebo: Be sure to use 16 ms as delay to allow the SpawnBind event to execute before this one.
	PostEventMS( &EV_PostSpawn, 16 );
}

void CBinaryFrobMover::ComputeAdditionalMembers()
{
	// angua: calculate the positions of the vertex  with the largest 
	// distance to the origin when the door is closed or open
	idClipModel *clipModel = GetPhysics()->GetClipModel();
	if (clipModel == NULL)
	{
		gameLocal.Error("Binary Frob Mover %s has no clip model", name.c_str());
	}
	idBox closedBox(clipModel->GetBounds(), m_ClosedOrigin, m_ClosedAngles.ToMat3());
	idVec3 closedBoxVerts[8];
	closedBox.GetVerts(closedBoxVerts);
	m_closedBox = closedBox; // grayman #720 - save for AI obstacle detection

	float maxDistSquare = 0;
	for (int i = 0; i < 8; i++)
	{
		float distSquare = (closedBoxVerts[i] - m_ClosedOrigin).LengthSqr();
		if (distSquare > maxDistSquare)
		{
			m_ClosedPos = closedBoxVerts[i] - m_ClosedOrigin;
			maxDistSquare = distSquare;
		}
	}
	//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin() + m_ClosedPos, GetPhysics()->GetOrigin() + m_ClosedPos + idVec3(0, 0, 30), 2, 200000);

	idBox openBox(clipModel->GetBounds(), m_OpenOrigin, m_OpenAngles.ToMat3());
	idVec3 openBoxVerts[8];
	openBox.GetVerts(openBoxVerts);

	maxDistSquare = 0;
	for (int i = 0; i < 8; i++)
	{
		float distSquare = (openBoxVerts[i] - m_OpenOrigin).LengthSqr();
		if (distSquare > maxDistSquare)
		{
			m_OpenPos = openBoxVerts[i] - m_OpenOrigin;
			maxDistSquare = distSquare;
		}
	}
	// gameRenderWorld->DebugArrow(colorRed, GetPhysics()->GetOrigin() + m_OpenPos, GetPhysics()->GetOrigin() + m_OpenPos + idVec3(0, 0, 30), 2, 200000);

	idRotation rot = m_Rotate.ToRotation();
	idVec3 rotationAxis = rot.GetVec();
	idVec3 normal = rotationAxis.Cross(m_ClosedPos);

	// grayman #3643 - normal should represent the door face, not a line
	// from the origin to the door closed position. Deal with normals that
	// are slightly off. Don't touch normals that have components that are
	// less than a multiple of 10 of each other. Ignore the z component.
	// This correction is important for thick doors that use controllers,
	// otherwise the door math thinks the controllers are both on the same
	// side of the door.

	if ( (normal.y != 0 ) && (abs(normal.x / normal.y) > 10.0f))
	{
		normal.y = 0;
	}
	else if ( (normal.x != 0) && (abs(normal.y / normal.x) > 10.0f))
	{
		normal.x = 0;
	}

	m_OpenDir = (m_OpenPos * normal) * normal;
	m_OpenDir.Normalize();
	// gameRenderWorld->DebugArrow(colorBlue, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + 20 * m_OpenDir, 2, 200000);
}

void CBinaryFrobMover::PostSpawn()
{
	// m_Translation is the vector between start position and end position
	m_Rotate = spawnArgs.GetAngles("rotate", "0 90 0");
	m_Translation = spawnArgs.GetVector("translate", "0 0 0");
	m_TransSpeed = spawnArgs.GetFloat( "translate_speed", "0");

	// angua: the origin of the door in opened and closed state
	m_ClosedOrigin = physicsObj.GetLocalOrigin();
	m_OpenOrigin = m_ClosedOrigin + m_Translation;

	m_ClosedAngles = physicsObj.GetLocalAngles();
	m_ClosedAngles.Normalize180();
	m_OpenAngles = m_ClosedAngles + m_Rotate;
	m_OpenAngles.Normalize180();

	if (m_ClosedOrigin.Compare(m_OpenOrigin) && m_ClosedAngles.Compare(m_OpenAngles))
	{
		gameLocal.Warning("FrobMover %s will not move, translation and rotation not set.", name.c_str());
	}

	// set up physics impulse behavior
	spawnArgs.GetFloat("impulse_thresh_open", "0", m_ImpulseThreshOpenSq );
	spawnArgs.GetFloat("impulse_thresh_close", "0", m_ImpulseThreshCloseSq );
	m_ImpulseThreshOpenSq *= m_ImpulseThreshOpenSq;
	m_ImpulseThreshCloseSq *= m_ImpulseThreshCloseSq;
	spawnArgs.GetVector("impulse_dir_open", "0 0 0", m_vImpulseDirOpen );
	spawnArgs.GetVector("impulse_dir_close", "0 0 0", m_vImpulseDirClose );
	if( m_vImpulseDirOpen.LengthSqr() > 0 )
		m_vImpulseDirOpen.Normalize();
	if( m_vImpulseDirClose.LengthSqr() > 0 )
		m_vImpulseDirClose.Normalize();

	// set the first intent according to the initial doorstate
	m_bIntentOpen = !m_Open;

	// Let the mapper override the initial frob intent on a partially opened door
	if( m_Open && spawnArgs.GetBool("first_frob_open") )
	{
		m_bIntentOpen = true;
		m_bInterrupted = true;
		m_StateChange = true;
	}

	// greebo: Partial Angles define the initial angles of the door
	idAngles partialAngles(0,0,0);

	// Check if the door should spawn as "open"
	if (m_Open)
	{
		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] shall be open at spawn time, checking start_* spawnargs.\r", name.c_str());

		// greebo: Load values from spawnarg, we might need to adjust them later on
		partialAngles = spawnArgs.GetAngles("start_rotate", "0 0 0");
		m_StartPos = spawnArgs.GetVector("start_position", "0 0 0");

		bool hasPartialRotation = !partialAngles.Compare(idAngles(0,0,0));
		bool hasPartialTranslation = !m_StartPos.Compare(idVec3(0,0,0));

		if (hasPartialRotation && !m_Translation.Compare(idVec3(0,0,0)))
		{
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] has partial angles set, calculating partial translation automatically.\r", name.c_str());

			// Sliding door has partial angles set, calculate the partial translation automatically
			idRotation maxRot = (m_OpenAngles - m_ClosedAngles).Normalize360().ToRotation();

			if (maxRot.GetAngle() > 0)
			{
				idRotation partialRot = partialAngles.Normalize360().ToRotation(); // grayman #720 - fixed partial rotation value
				float fraction = partialRot.GetAngle() / maxRot.GetAngle();
				m_StartPos = m_Translation * fraction;
			}
			else
			{
				gameLocal.Warning("Mover '%s' has start_rotate set, but rotation angles are zero.", name.c_str());
				DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("[%s] has start_rotate set, but rotation angles are zero.\r", name.c_str());
			}
		}
		else if (hasPartialTranslation && !m_Rotate.Compare(idAngles(0,0,0)))
		{
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] has partial translation set, calculating partial rotation automatically.\r", name.c_str());

			// Rotating door has partial translation set, calculate the partial rotation automatically
			float maxTrans = (m_OpenOrigin - m_ClosedOrigin).Length();
			float partialTrans = m_StartPos.Length();

			if (maxTrans > 0)
			{
				float fraction = partialTrans / maxTrans;

				partialAngles = m_ClosedAngles + (m_OpenAngles - m_ClosedAngles) * fraction;
			}
			else
			{
				gameLocal.Warning("Mover '%s' has start_position set, but translation is zero.", name.c_str());
				DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("[%s] has partial translation set, but translation is zero?\r", name.c_str());
			}
		}
		else if (!hasPartialRotation && !hasPartialTranslation)
		{
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s]: has open='1' but neither partial translation nor rotation are set, assuming fully opened mover.\r", name.c_str());
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s]: Resetting first_frob_open and interrupted flags.\r", name.c_str());
			
			// Neither start_position nor start_rotate set, assume fully opened door
			partialAngles = m_OpenAngles - m_ClosedAngles;
			m_StartPos = m_OpenOrigin - m_ClosedOrigin;

			// greebo: In this case, first_frob_open makes no sense, override the settings
			m_bIntentOpen = false;
			m_bInterrupted = false;
			m_StateChange = false; 
		}
	}

	ComputeAdditionalMembers();

	if (m_Open) 
	{
		// door starts out partially open, set origin and angles to the values defined in the spawnargs.
		physicsObj.SetLocalOrigin(m_ClosedOrigin + m_StartPos);
		physicsObj.SetLocalAngles(m_ClosedAngles + partialAngles);
	}

	UpdateVisuals();

	// grayman #2603 - Process targets. For those that are lights, add yourself
	// to their switch list.
	// grayman #3643 - For those that are doors, add yourself to
	// their controller list.

	for ( int i = 0 ; i < targets.Num() ; i++ )
	{
		idEntity* e = targets[i].GetEntity();
		if (e)
		{
			if (e->IsType(idLight::Type))
			{
				idLight* light = static_cast<idLight*>(e);
				light->AddSwitch(this);
			}
			else if (e->IsType(CFrobDoor::Type))
			{
				// The door needs to accommodate an AI walking through it
				// to have this switch treated as a door controller.
				idVec3 size = e->GetPhysics()->GetAbsBounds().GetSize();
				if ( (size.z >= 87) && ((size.x > 32) || (size.y > 32)) )
				{
					CFrobDoor* door = static_cast<CFrobDoor*>(e);
					door->AddController(this);
				}
			}
		}
	}

	// Check if we should auto-open, which could also happen right at the map start
	if (IsAtClosedPosition())
	{
		float autoOpenTime = -1;
		if (spawnArgs.GetFloat("auto_open_time", "-1", autoOpenTime) && autoOpenTime >= 0)
		{
			// Convert the time to msec and post the event
			PostEventMS(&EV_TDM_FrobMover_Open, static_cast<int>(SEC2MS(autoOpenTime)));
		}
	}
	else if (IsAtOpenPosition())
	{
		// Check if we should move back to the closedpos
		float autoCloseTime = -1;
		if (spawnArgs.GetFloat("auto_close_time", "-1", autoCloseTime) && autoCloseTime >= 0)
		{
			// Convert the time to msec and post the event
			PostEventMS(&EV_TDM_FrobMover_Close, static_cast<int>(SEC2MS(autoCloseTime)));
		}
	}
}

void CBinaryFrobMover::Event_PostSpawn() 
{
	// Call the virtual function
	PostSpawn();
}

void CBinaryFrobMover::Lock(bool bMaster)
{
	if (!PreLock(bMaster)) {
		// PreLock returned FALSE, cancel the operation
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("[%s] FrobMover prevented from being locked\r", name.c_str());
		return;
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("[%s] FrobMover is locked\r", name.c_str());

	m_Lock->SetLocked(true);
	CallStateScript();

	// Fire the event for the subclasses
	OnLock(bMaster);
}

void CBinaryFrobMover::TellRegisteredUsers()
{
	// grayman #1145 - remove this door's area number from each registered AI's forbidden area list

	int numUsers = m_registeredAI.Num();

	for (int i = 0 ; i < numUsers ; i++)
	{
		idAI* ai = m_registeredAI[i].GetEntity();
		if (!ai)	//stgatilov #5318: AI already died
			continue;
		idAAS* aas = ai->GetAAS();
		if (aas != NULL)
		{
			gameLocal.m_AreaManager.RemoveForbiddenArea(GetAASArea(aas),ai);
		}
	}
	m_registeredAI.Clear(); // served its purpose, clear for next batch
}

// grayman #3643 - return 'true' if the caller should open the door, 'false' if not
bool CBinaryFrobMover::Unlock(bool bMaster)
{
	if (!PreUnlock(bMaster)) {
		// PreUnlock returned FALSE, cancel the operation
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("[%s] FrobMover prevented from being unlocked\r", name.c_str());
		return false; // grayman #3643
	}
	
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("[%s] FrobMover is unlocked\r", name.c_str());

	m_Lock->SetLocked(false);
	CallStateScript();

	// Fire the event for the subclasses
	bool shouldOpenDoor = !OnUnlock(bMaster); // grayman #3643

	TellRegisteredUsers(); // grayman #1145 - remove this door's area number from each registered AI's forbidden area list
	return shouldOpenDoor;
}

void CBinaryFrobMover::ToggleLock()
{
	// greebo: When the mover is open, close and lock it.
	if (m_Open == true)
	{
		Close();
	}

	if (m_Lock->IsLocked())
	{
		Unlock();
	}
	else
	{
		Lock();
	}
}

void CBinaryFrobMover::CloseAndLock()
{
	if (!IsLocked() && !IsAtClosedPosition())
	{
		m_LockOnClose = true;
		Close();
	}
	else
	{
		// We're at close position, just lock it
		Lock();
	}
}

bool CBinaryFrobMover::StartMoving(bool open) 
{
	// Get the target position and orientation
	idVec3 targetOrigin = open ? m_OpenOrigin : m_ClosedOrigin;
	idAngles targetAngles = open ? m_OpenAngles : m_ClosedAngles;

	// Assume we are moving
	m_Rotating = true;
	m_Translating = true;

	// Clear the block variables
	m_StoppedDueToBlock = false;
	m_LastBlockingEnt = NULL;
	m_bInterrupted = false;
	
	idAngles angleDelta = (targetAngles - physicsObj.GetLocalAngles()).Normalize180();

	if (!angleDelta.Compare(idAngles(0,0,0)))
	{
		Event_RotateOnce(angleDelta);
	}
	else
	{
		m_Rotating = false; // nothing to rotate
	}
	
	if (!targetOrigin.Compare(physicsObj.GetLocalOrigin(), VECTOR_EPSILON))
	{	
		if (m_TransSpeed)
		{
			Event_SetMoveSpeed(m_TransSpeed);
		}

		MoveToLocalPos(targetOrigin); // Start moving
	}
	else
	{
		m_Translating = false;
	}

	// Now let's look if we are actually moving
	m_StateChange = (m_Translating || m_Rotating);

	if (m_StateChange)
	{
		// Fire the "we're starting to move" event
		OnMoveStart(open);

		// If we're changing states from closed to open, set the bool right now
		if (!m_Open)
		{
			m_Open = true;
		}
	}

	return m_StateChange;
}

void CBinaryFrobMover::Open(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("BinaryFrobMover: Opening\r" );

	if (!PreOpen()) 
	{
		// PreOpen returned FALSE, cancel the operation
		return;
	}

	// We have green light, let's see where we are starting from
	bool wasClosed = IsAtClosedPosition();

	// Now, actually trigger the moving process
	if (StartMoving(true))
	{
		// We're moving, fire the event and pass the starting state
		OnStartOpen(wasClosed, bMaster);
	}

	// Set the "intention" flag so that we're closing next time, even if we didn't move
	m_bIntentOpen = false;
}

void CBinaryFrobMover::Close(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("BinaryFrobMover: Closing\r" );

	if (!PreClose()) 
	{
		// PreClose returned FALSE, cancel the operation
		return;
	}

	// We have green light, let's see where we are starting from
	bool wasOpen = IsAtOpenPosition();

	// Now, actually trigger the moving process
	if (StartMoving(false))
	{
		// We're moving, fire the event and pass the starting state
		OnStartClose(wasOpen, bMaster);
	}

	// Set the "intention" flag so that we're opening next time, even if we didn't move
	m_bIntentOpen = true;
}

// grayman #3643 - Allow keys and lockpicks to be used on buttons/levers (door controllers)

bool CBinaryFrobMover::UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	if (item == NULL)
	{
		return false;
	}

	// Pass the call on to the master, if we have one
	if (GetFrobMaster() != NULL) 
	{
		return GetFrobMaster()->UseByItem(impulseState, item);
	}

	// Call the used_action_script via the generic idEntity method, if desired by the mapper
	if ( spawnArgs.GetBool("call_used_action_script", "0") )
	{
		idEntity::UseByItem(impulseState, item);
	}

	assert(item->Category() != NULL);

	// Retrieve the entity behind that item and reject NULL entities
	idEntity* itemEntity = item->GetItemEntity();
	if (itemEntity == NULL)
	{
		return false;
	}

	// Get the name of this inventory category
	const idStr& itemName = item->Category()->GetName();
	
	if ((itemName == "#str_02392") && (impulseState == EPressed )) // Keys
	{
		// Keys can be used on button PRESS event, let's see if the key matches
		if (m_UsedByName.FindIndex(itemEntity->name) != -1)
		{
			// If we're locked or closed, just toggle the lock. 
			if (IsLocked() || IsAtClosedPosition())
			{
				ToggleLock();
			}
			// If we're open, set a lock request and start closing.
			else
			{
				// Close the door and set the lock request to true
				CloseAndLock();
			}

			return true;
		}
		else
		{
			FrobMoverStartSound("snd_wrong_key");
			return false;
		}
	}
	// grayman #4262 - Handle key repeat and key release. The desired
	// response was handled by the key press above.
	else if ((itemName == "#str_02392") && ((impulseState == ERepeat ) || (impulseState == EReleased ))) // Keys
	{
		return false;
	}
	else if (itemName == "#str_02389" ) // Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks are different, we need to look at the button state
		// First we check if this item is a lockpick. It has to be of the toolclass lockpick
		// and the type must be set.
		idStr str = itemEntity->spawnArgs.GetString("lockpick_type", "");

		if (str.Length() == 1)
		{
			// greebo: Check if the item owner is a player, and if yes, 
			// update the immobilization flags.
			idEntity* itemOwner = item->GetOwner();

			if (itemOwner->IsType(idPlayer::Type))
			{
				idPlayer* playerOwner = static_cast<idPlayer*>(itemOwner);
				playerOwner->SetImmobilization("Lockpicking", EIM_ATTACK);

				// Schedule an event 1/3 sec. from now, to enable weapons again after this time
				CancelEvents(&EV_TDM_FrobMover_ClearPlayerImmobilization);
				PostEventMS(&EV_TDM_FrobMover_ClearPlayerImmobilization, 300, playerOwner);
			}

			// Pass the call to the lockpick routine
			return m_Lock->ProcessLockpickImpulse(impulseState, static_cast<int>(str[0]));
		}
		else
		{
			gameLocal.Warning("Wrong 'type' spawnarg for lockpicking on item %s, must be a single character.", itemEntity->name.c_str());
			return false;
		}
	}

	// grayman #3643 - if you get here, pass to the default method
	return idEntity::UseByItem(impulseState, item);
}

// grayman #3643 - Allow keys and lockpicks to be used on buttons/levers (door controllers)

bool CBinaryFrobMover::CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) 
{
	// First, check if the frob master can be used
	// If this doesn't succeed, perform additional checks
	idEntity* master = GetFrobMaster();
	if ( master != NULL && master->CanBeUsedByItem(item, isFrobUse) )
	{
		return true;
	}

	if (item == NULL)
	{
		return false;
	}

	assert(item->Category() != NULL);

	// FIXME: Move this to idEntity to some sort of "usable_by_inv_category" list?
	const idStr& itemName = item->Category()->GetName();
	if (itemName == "#str_02392" ) // Keys
	{
		// Keys can always be used on doors
		// Exception: for "frob use" this only applies when the door is locked
		return (isFrobUse) ? IsLocked() : true;
	}
	else if (itemName == "#str_02389" ) // Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks behave similar to keys
		return (isFrobUse) ? IsLocked() : true;
	}

	// grayman #3643 - if you get here, pass to the default method
	return idEntity::CanBeUsedByItem(item, isFrobUse);
}

void CBinaryFrobMover::ToggleOpen()
{
	if (!IsMoving())
	{
		// We are not moving
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("BinaryFrobMover: Was stationary on ToggleOpen\r" );

		if (m_bIntentOpen)
		{
			Open();
		}
		else
		{
			Close();
		}

		return;
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Was moving on ToggleOpen.\r" );

	// We are moving, is the mover interruptable?
	if (m_bInterruptable && PreInterrupt())
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Interrupted! Stopping door\r" );

		m_bInterrupted = true;
		Event_StopRotating();
		Event_StopMoving();

		OnInterrupt();
	}
}

void CBinaryFrobMover::DoneMoving()
{
	idMover::DoneMoving();
    m_Translating = false;

	DoneStateChange();
}

void CBinaryFrobMover::DoneRotating()
{
	idMover::DoneRotating();
    m_Rotating = false;

	DoneStateChange();
}

void CBinaryFrobMover::DoneStateChange()
{
	if (!m_StateChange || IsMoving())
	{
		// Entity is still moving, wait for the next call (this usually gets called twice)
		return;
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("BinaryFrobMover: DoneStateChange\r" );

	// Check which position we're at, set the state variables and fire the correct events

	m_StateChange = false;

	// We are assuming that we're still "open", this gets overwritten by the check below
	// if we are at the final "closed" position
	m_Open = true;

	if (IsAtClosedPosition())
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closed completely\r" );

		m_Open = false;

		// We have reached the final close position, fire the event
		OnClosedPositionReached();
	}
	else if (IsAtOpenPosition())
	{
		// We have reached the final open position, fire the event
		OnOpenPositionReached();
	}

	// Invoked the script object to notify it about the state change
	CallStateScript();
}

bool CBinaryFrobMover::IsAtOpenPosition()
{
	const idVec3& localOrg = physicsObj.GetLocalOrigin();

	const idAngles& localAngles = physicsObj.GetLocalAngles();
	
	// greebo: Let the check be slightly inaccurate (use the standard epsilon).
	return (localAngles - m_OpenAngles).Normalize180().Compare(ang_zero, VECTOR_EPSILON) && 
		   localOrg.Compare(m_OpenOrigin, VECTOR_EPSILON);
}

bool CBinaryFrobMover::IsAtClosedPosition()
{
	const idVec3& localOrg = physicsObj.GetLocalOrigin();

	const idAngles& localAngles = physicsObj.GetLocalAngles();

	// greebo: Let the check be slightly inaccurate (use the standard epsilon).
	return (localAngles - m_ClosedAngles).Normalize180().Compare(ang_zero, VECTOR_EPSILON) && 
		   localOrg.Compare(m_ClosedOrigin, VECTOR_EPSILON);
}

void CBinaryFrobMover::CallStateScript()
{
	idStr functionName = spawnArgs.GetString("state_change_callback", "");

	if (!functionName.IsEmpty())
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Callscript '%s' Open: %d, Locked: %d Interrupted: %d\r",
			functionName.c_str(), m_Open, m_Lock->IsLocked(), m_bInterrupted);
		CallScriptFunctionArgs(functionName, true, 0, "ebbb", this, m_Open, m_Lock->IsLocked(), m_bInterrupted);
	}
}

void CBinaryFrobMover::Event_Activate(idEntity *activator) 
{
	ToggleOpen();
}

void CBinaryFrobMover::OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity)
{
	m_LastBlockingEnt = blockingEntity;
	// greebo: If we're blocked by something, check if we should stop moving

	if (( gameLocal.time >= m_nextBounceTime ) && m_stopWhenBlocked)
	{
		m_bInterrupted = true;
		m_StoppedDueToBlock = true;

		Event_StopRotating();
		Event_StopMoving();

		// grayman #3755 - bounce off humanoid AI?
		if ( blockingEntity->IsType(idAI::Type) && IsType(CFrobDoor::Type) )
		{
			if (blockingEntity->GetPhysics()->GetMass() > SMALL_AI_MASS)
			{
				m_nextBounceTime = gameLocal.time + 1000; // next time you can bounce

				idAI* beAI = static_cast<idAI*>(blockingEntity);
				// grayman #3756 - if AI is moving, the door bounces off him
				if ( beAI->AI_FORWARD )
				{
					static_cast<CFrobDoor*>(this)->PushDoorHard();

					StartMoving(true); // reverse direction

					// Set the "intention" flag to its opposite
					m_bIntentOpen = !m_bIntentOpen;
				}

				// grayman #3756 - Alert the AI that a door hit him. We're going
				// to treat this as a suspicious door, so make sure there's a record
				// of who set it in motion.
				idEntity* lastUsedBy = static_cast<CFrobDoor*>(this)->GetLastUsedBy();
				if (lastUsedBy == NULL)
				{
					// _Someone_ set the door in motion, so if lastUsedBy is NULL,
					// it had to be the player.
					lastUsedBy = gameLocal.GetLocalPlayer();
				}
				m_SetInMotionByActor = static_cast<idActor*>(lastUsedBy);
				beAI->TactileAlert(this);
			}
		}
	}

	// Clear the close request flag
	m_LockOnClose = false;
	CancelEvents(&EV_TDM_FrobMover_HandleLockRequest);
}

void CBinaryFrobMover::ApplyImpulse(idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse)
{
	idVec3 SwitchDir;
	float SwitchThreshSq(0), ImpulseMagSq(0);

	if ( (m_Open && m_ImpulseThreshCloseSq > 0) || (!m_Open && m_ImpulseThreshOpenSq > 0) )
	{
		if (m_Open)
		{
			SwitchDir = m_vImpulseDirClose;
			SwitchThreshSq = m_ImpulseThreshCloseSq;
		}
		else
		{
			SwitchDir = m_vImpulseDirOpen;
			SwitchThreshSq = m_ImpulseThreshOpenSq;
		}
		
		// only resolve along the axis if it is set.  Defaults to (0,0,0) if not set
		if (SwitchDir.LengthSqr())
		{
			SwitchDir = GetPhysics()->GetAxis() * SwitchDir;
			ImpulseMagSq = impulse*SwitchDir;
			ImpulseMagSq *= ImpulseMagSq;
		}
		else
		{
			ImpulseMagSq = impulse.LengthSqr();
		}

		if (ImpulseMagSq >= SwitchThreshSq)
		{
			ToggleOpen();
		}
	}

	idEntity::ApplyImpulse( ent, id, point, impulse);
}

/*-------------------------------------------------------------------------*/

bool CBinaryFrobMover::IsMoving()
{
	return (m_Translating || m_Rotating);
}

/*-------------------------------------------------------------------------*/

bool CBinaryFrobMover::IsChangingState()
{
	return m_StateChange;
}

/*-------------------------------------------------------------------------*/

void CBinaryFrobMover::GetRemainingMovement(idVec3& out_deltaPosition, idAngles& out_deltaAngles)
{
	// Get remaining translation if translating
	if (m_bIntentOpen)
	{
		out_deltaPosition = (m_OpenOrigin + m_Translation) - physicsObj.GetOrigin(); // grayman #2345
//		out_deltaPosition = (m_StartPos + m_Translation) - physicsObj.GetOrigin(); // grayman #2345
	}
	else
	{
		out_deltaPosition = m_ClosedOrigin - physicsObj.GetOrigin(); // grayman #2345
//		out_deltaPosition = m_StartPos - physicsObj.GetOrigin(); // grayman #2345
	}

	// Get remaining rotation
	idAngles curAngles;
	physicsObj.GetAngles(curAngles);

	if (m_bIntentOpen)
	{
		out_deltaAngles = m_OpenAngles - curAngles;
	}
	else
	{
		out_deltaAngles = m_ClosedAngles - curAngles;
	}

	// Done
}

// grayman #3462
int CBinaryFrobMover::GetMoveStartTime()
{
	return m_timeDoorStartedMoving;
}

// grayman #3462
int CBinaryFrobMover::GetMoveTime()
{
	return move_time;
}

// grayman #3755
void CBinaryFrobMover::SetTransSpeed(float speed)
{
	m_TransSpeed = speed;
}

// grayman #3755
float CBinaryFrobMover::GetTransSpeed()
{
	return m_TransSpeed;
}

float CBinaryFrobMover::GetMoveTimeRotationFraction() // grayman #3711
{
	// Get the current angles
	const idAngles& curAngles = physicsObj.GetLocalAngles();

	// Calculate the delta
	idAngles delta = dest_angles - curAngles;
	delta[0] = idMath::Fabs(delta[0]);
	delta[1] = idMath::Fabs(delta[1]);
	delta[2] = idMath::Fabs(delta[2]);

	// greebo: Note that we don't need to compare against zero angles here, because
	// this code won't be called in this case (see idMover::BeginRotation).

	idAngles fullRotation = (m_OpenAngles - m_ClosedAngles).Normalize180();
	fullRotation[0] = idMath::Fabs(fullRotation[0]);
	fullRotation[1] = idMath::Fabs(fullRotation[1]);
	fullRotation[2] = idMath::Fabs(fullRotation[2]);

	// Get the maximum angle component
	int index = (delta[0] > delta[1]) ? 0 : 1;
	index = (delta[2] > delta[index]) ? 2 : index;

	if (fullRotation[index] < idMath::FLT_EPS) return 1;

	float fraction = delta[index]/fullRotation[index];

	return fraction;
}

float CBinaryFrobMover::GetMoveTimeTranslationFraction() // grayman #3711
{
	// grayman #3643 - this is only useful for doors

	if (!IsType(CFrobDoor::Type))
	{
		return 1.0f;
	}

	// Get the current origin
	const idVec3& curOrigin = physicsObj.GetLocalOrigin(); // grayman #4015 - use local origin
	//const idVec3& curOrigin = physicsObj.GetOrigin();

	// Calculate the delta
	idVec3 delta = dest_position - curOrigin;
	delta[0] = idMath::Fabs(delta[0]);
	delta[1] = idMath::Fabs(delta[1]);
	delta[2] = idMath::Fabs(delta[2]);

	idVec3 fullTranslation = m_OpenOrigin - m_ClosedOrigin;
	fullTranslation[0] = idMath::Fabs(fullTranslation[0]);
	fullTranslation[1] = idMath::Fabs(fullTranslation[1]);
	fullTranslation[2] = idMath::Fabs(fullTranslation[2]);

	// Get the maximum translation component
	int index = (delta[0] > delta[1]) ? 0 : 1;
	index = (delta[2] > delta[index]) ? 2 : index;

	if (fullTranslation[index] < idMath::FLT_EPS)
	{
		return 1;
	}

	float fraction = delta[index]/fullTranslation[index];

	return fraction;
}

int CBinaryFrobMover::GetAASArea(idAAS* aas)
{
	if (aas == NULL)
	{
		return -1;
	}

	if (GetPhysics() == NULL)
	{
		return -1;
	}

	idClipModel *clipModel = GetPhysics()->GetClipModel();
	if (clipModel == NULL)
	{
		gameLocal.Error("FrobMover %s has no clip model", name.c_str());
	}

	const idBounds& bounds = clipModel->GetAbsBounds();

	idVec3 center = GetClosedBox().GetCenter(); // grayman #2877 - new way
//	idVec3 center = GetPhysics()->GetOrigin() + m_ClosedPos * 0.5; // grayman #2877 - old way
	center.z = bounds[0].z + 1;

	int areaNum = aas->PointReachableAreaNum( center, bounds, AREA_REACHABLE_WALK );
	idAASLocal* aasLocal = dynamic_cast<idAASLocal*> (aas);

	if (aasLocal)
	{
		int clusterNum = aasLocal->GetClusterNum(areaNum);
		if (clusterNum > 0)
		{
			// angua: This is not a portal area, check the surroundings for portals
			idReachability *reach;
			for ( reach = aasLocal->GetAreaFirstReachability(areaNum); reach; reach = reach->next ) {
				int testAreaNum = reach->toAreaNum;
				int testClusterNum = aasLocal->GetClusterNum(testAreaNum);
				if (testClusterNum < 0)
				{
					// we have found a portal area, take this one instead
					areaNum = testAreaNum;
					break;
				}
			}
		}

		// aasLocal->DrawArea(areaNum);
	}

//	idStr areatext(areaNum);
//	gameRenderWorld->DebugLine(colorGreen,center,center + idVec3(0,0,20),10000000);
//	gameRenderWorld->DebugLine(colorOrange,GetPhysics()->GetOrigin(),GetPhysics()->GetOrigin() + m_ClosedPos,10000000);
//	gameRenderWorld->DebugText(areatext.c_str(), center + idVec3(0,0,1), 0.2f, colorGreen, mat3_identity, 1, 10000000);

	return areaNum;
}

void CBinaryFrobMover::OnMoveStart(bool opening)
{
	// Clear this door from the ignore list so AI can react to it again.
	// grayman #2859 - but only if the door was closed and is now opening

	if ( opening )
	{
		ClearStimIgnoreList(ST_VISUAL);
		EnableStim(ST_VISUAL);
	}

	// grayman #3462 - note when door started to move
	m_timeDoorStartedMoving = gameLocal.time;
}

bool CBinaryFrobMover::PreOpen() 
{
	if (m_Lock->IsLocked()) 
	{
		// Play the "I'm locked" sound 
		FrobMoverStartSound("snd_locked");
		// and prevent the door from opening (return false)
		return cv_door_ignore_locks.GetBool(); // 2.10: introduced a new beta tester cvar: cv_door_ignore_locks
	}

	return true; // default: mover is allowed to open
}

bool CBinaryFrobMover::PreClose()
{
	return true; // default: mover is allowed to close
}

bool CBinaryFrobMover::PreInterrupt()
{
	return true; // default: mover is allowed to be interrupted
}

bool CBinaryFrobMover::PreLock(bool bMaster)
{
	return true; // default: mover is allowed to be locked
}

bool CBinaryFrobMover::PreUnlock(bool bMaster)
{
	return true; // default: mover is allowed to be unlocked
}

void CBinaryFrobMover::OnStartOpen(bool wasClosed, bool bMaster)
{
	if (wasClosed)
	{
		// Only play the "open" sound when the door was completely closed
		FrobMoverStartSound("snd_open");

		// trigger our targets on opening, if set to do so
		if (spawnArgs.GetBool("trigger_on_open", "0"))
		{
			ActivateTargets(this);
		}
	}

	// Clear the lock request flag in any case
	m_LockOnClose = false;
	CancelEvents(&EV_TDM_FrobMover_HandleLockRequest);
}

void CBinaryFrobMover::OnStartClose(bool wasOpen, bool bMaster)
{
	// To be implemented by the subclasses
}

void CBinaryFrobMover::OnOpenPositionReached()
{
	TellRegisteredUsers(); // grayman #1145

	// play the opened sound when the door opens completely
	FrobMoverStartSound("snd_opened"); // grayman #3263

	if (spawnArgs.GetBool("trigger_when_opened", "0"))
	{
		// trigger our targets when completely opened, if set to do so
		ActivateTargets(this);
	}

	// Check if we should move back to the closedpos after use
	float autoCloseTime = -1;
	if (spawnArgs.GetFloat("auto_close_time", "-1", autoCloseTime) && autoCloseTime >= 0)
	{
		// Convert the time to msec and post the event
		PostEventMS(&EV_TDM_FrobMover_Close, static_cast<int>(SEC2MS(autoCloseTime)));
	}
}

void CBinaryFrobMover::OnClosedPositionReached()
{
	// play the closing sound when the door closes completely
	FrobMoverStartSound("snd_close");

	// trigger our targets on completely closing, if set to do so
	if (spawnArgs.GetBool("trigger_on_close", "0"))
	{
		ActivateTargets(this);
	}

	// Check if we should move back to the openpos
	float autoOpenTime = -1;
	if (spawnArgs.GetFloat("auto_open_time", "-1", autoOpenTime) && autoOpenTime >= 0)
	{
		// Convert the time to msec and post the event
		PostEventMS(&EV_TDM_FrobMover_Open, static_cast<int>(SEC2MS(autoOpenTime)));
	}

	// Do we have a close request?
	if (m_LockOnClose)
	{
		// Clear the flag, regardless what happens
		m_LockOnClose = false;
		
		// Post a lock request in LOCK_REQUEST_DELAY msecs
		PostEventMS(&EV_TDM_FrobMover_HandleLockRequest, LOCK_REQUEST_DELAY);
	}
}

void CBinaryFrobMover::OnInterrupt()
{
	// Clear the close request flag
	m_LockOnClose = false;
	CancelEvents(&EV_TDM_FrobMover_HandleLockRequest);
}

void CBinaryFrobMover::OnLock(bool bMaster)
{
	FrobMoverStartSound("snd_lock");

	m_Lock->OnLock();
}

// grayman #3643 - report back whether OnUnlock() automatically opens the door
bool CBinaryFrobMover::OnUnlock(bool bMaster)
{
	bool doorOpened = false;

	m_Lock->OnUnlock();

	int soundLength = FrobMoverStartSound("snd_unlock");

	// angua: only open the master
	// only if the other part is an openpeer it will be opened by ToggleOpen
	// otherwise it will stay closed

	/*  grayman #3643 - Here is the bug. The door has begun to move before the unlocking sound
		completes. When the sound completes, it toggles the door, expecting
		that to cause the open to begin. Since the door is already moving,
		this causes the door to stop moving, and think it's been interruped.
	*/
	if (cv_door_auto_open_on_unlock.GetBool() && bMaster)
	{
		// The configuration says: open the mover when it's unlocked, but let's check the mapper's settings
		bool openOnUnlock = true;
		bool spawnArgSet = spawnArgs.GetBool("open_on_unlock", "1", openOnUnlock);

		if (!spawnArgSet || openOnUnlock)
		{
			// No spawnarg set or opening is allowed, just open the mover after a short delay
			PostEventMS(&EV_TDM_FrobMover_ToggleOpen, soundLength);
			doorOpened = true; // grayman #3643
		}
	}

	return doorOpened; // grayman #3643
}

int CBinaryFrobMover::FrobMoverStartSound(const char* soundName)
{
	// Default implementation: Just play the sound on this entity.
	int length = 0;
	StartSound(soundName, SND_CHANNEL_ANY, 0, false, &length);

	return length;
}

void CBinaryFrobMover::Event_Open()
{
	Open();
}

void CBinaryFrobMover::Event_Close()
{
	Close();
}

void CBinaryFrobMover::Event_ToggleOpen()
{
	ToggleOpen();
}

void CBinaryFrobMover::Event_IsOpen()
{
	idThread::ReturnInt(m_Open);
}

void CBinaryFrobMover::Event_Lock()
{
	Lock();
}

void CBinaryFrobMover::Event_Unlock()
{
	Unlock();
}

void CBinaryFrobMover::Event_ToggleLock()
{
	ToggleLock();
}

void CBinaryFrobMover::Event_IsLocked()
{
	idThread::ReturnInt(IsLocked());
}

void CBinaryFrobMover::Event_IsPickable()
{
	idThread::ReturnInt(IsPickable());
}

idVec3 CBinaryFrobMover::GetCurrentPos()
{
	idVec3 closedDir = m_ClosedPos;
	closedDir.z = 0;
	float length = closedDir.LengthFast();

	// grayman #720 - previous version was giving the wrong result for a
	// NS door partially opened clockwise. Corrected by normalizing the
	// closed angle and using abs() on the cos() and sin() calcs and letting
	// closedDir and m_OpenDir set the signs when calculating currentPos.

	idAngles angles = physicsObj.GetLocalAngles();
	idAngles closedAngles = GetClosedAngles();
	idAngles deltaAngles = angles - closedAngles.Normalize360();
	idRotation rot = deltaAngles.ToRotation();
	float alpha = idMath::Fabs(rot.GetAngle());
	
	idVec3 currentPos = GetPhysics()->GetOrigin() 
		+ closedDir * idMath::Fabs(idMath::Cos(alpha * idMath::PI / 180))
		+ m_OpenDir * length * idMath::Fabs(idMath::Sin(alpha * idMath::PI / 180));

	return currentPos;
}

float CBinaryFrobMover::GetFractionalPosition()
{
	// Don't know if a door translates or rotates or both,
	// so look at fractional movement of both and take the max?
	float returnval(0.0f);
	const idVec3& localOrg = physicsObj.GetLocalOrigin();
	const idAngles& localAngles = physicsObj.GetLocalAngles();
	
	// check for non-zero rotation first
	// grayman #3042 - normalize to 180, not 360
	float maxRotAngle = (m_OpenAngles - m_ClosedAngles).Normalize180().ToRotation().GetAngle();
	float maxSlideDistance = (m_OpenOrigin - m_ClosedOrigin).Length();
	if ( maxRotAngle != 0 )
	{
		idRotation curRot = (localAngles - m_ClosedAngles).Normalize180().ToRotation();
		returnval = curRot.GetAngle() / maxRotAngle;
	}
	else if ( maxSlideDistance != 0 )
	{
		// if door doesn't have rotation, check translation
		returnval = (localOrg - m_ClosedOrigin).Length() / maxSlideDistance;
	}
	else {
		//this should not happen during gameplay
		//however, it happens on map start for double doors
		//when door A is post-spawned, it calls this on door B before that is post-spawned
		returnval = 0.5;
	}

	return returnval;
}

void CBinaryFrobMover::SetFractionalPosition(float fraction, bool immediately)
{
	idVec3 targetOrigin = m_ClosedOrigin + (m_OpenOrigin - m_ClosedOrigin) * fraction;
	idAngles targetAngles = m_ClosedAngles + (m_OpenAngles - m_ClosedAngles) * fraction;
	idAngles angleDelta = (targetAngles - physicsObj.GetLocalAngles()).Normalize180();

	if (immediately) {
		//stgatilov #5683: immediate move for hot-reload purposes
		physicsObj.SetLocalOrigin(targetOrigin);
		physicsObj.SetLocalAngles(targetAngles);
	}
	else
	{
		if (!angleDelta.Compare(ang_zero, 0.01f))
		{
			Event_RotateOnce(angleDelta);
		}

		MoveToLocalPos(targetOrigin);
	}

	UpdateVisuals();
}

void CBinaryFrobMover::Event_HandleLockRequest()
{
	// Check if we are at our "closed" position, if yes: lock, if no: postpone the event
	if (IsAtClosedPosition())
	{
		// Yes, we are at our "closed" position, lock ourselves
		Lock(true);
	}
	else
	{
		// Not at closed position (yet), postpone the event
		PostEventMS(&EV_TDM_FrobMover_HandleLockRequest, LOCK_REQUEST_DELAY);
	}
}

void CBinaryFrobMover::FrobAction(bool frobMaster, bool isFrobPeerAction)
{
	idEntity::FrobAction( frobMaster, isFrobPeerAction );
	if( m_bInterruptable && cv_tdm_door_control.GetBool() )
		m_bFineControlStarting = true;
}

void CBinaryFrobMover::FrobHeld(bool frobMaster, bool isFrobPeerAction, int holdTime)
{
	if ( !m_bInterruptable || !cv_tdm_door_control.GetBool() || holdTime < 200 )
		return;

	idPlayer *player = gameLocal.GetLocalPlayer();
	
	if( m_bFineControlStarting )
	{
		// initialize fine control
		player->SetImmobilization( "door handling",  EIM_VIEW_ANGLE );
		m_mousePosition.x = player->usercmd.mx;
		m_mousePosition.y = player->usercmd.my;

		// Stop the door from opening or closing as normal:
		m_bInterrupted = true;
		Event_StopRotating();
		Event_StopMoving();
		OnInterrupt();

		m_bFineControlStarting = false;
	}

	//float dx = player->usercmd.mx - m_mousePosition.x;
	float dy = player->usercmd.my - m_mousePosition.y;
	m_mousePosition.x = player->usercmd.mx;
	m_mousePosition.y = player->usercmd.my;

	// figure out the view direction (rotation only for now)
	float sign = 1.0f;
	idRotation openRot = (0.1f*(m_OpenAngles - m_ClosedAngles).Normalize360()).ToRotation();
	openRot.SetOrigin(GetPhysics()->GetOrigin());
	idVec3 playerOrg = player->GetPhysics()->GetOrigin();
	idVec3 testOrg = playerOrg;
	openRot.RotatePoint(testOrg);
	float viewDot = (testOrg - playerOrg) * player->viewAxis[0];
	if (viewDot < 0)
		sign = 1.0f;
	else
		sign = -1.0f;

	float desiredPos = GetFractionalPosition() + sign * cv_tdm_door_control_sensitivity.GetFloat() * dy;
	desiredPos = idMath::ClampFloat( 0.0f, 1.0f, desiredPos );
	SetFractionalPosition( desiredPos, false );
}

void CBinaryFrobMover::FrobReleased(bool frobMaster, bool isFrobPeerAction, int holdTime)
{
	idPlayer *player = gameLocal.GetLocalPlayer();
	player->SetImmobilization( "door handling",  0 );
}

// grayman #1145 - add an AI who unsuccessfully tried to open a locked door

void CBinaryFrobMover::RegisterAI(idAI* ai)
{
	idEntityPtr<idAI> aiPtr;
	aiPtr = ai;
	if (auto dupe = m_registeredAI.Find(aiPtr))
		return;
	m_registeredAI.Append(aiPtr);
}

// grayman #2691 - get the axis of rotation

idVec3 CBinaryFrobMover::GetRotationAxis()
{
	// grayman #2866 - ToRotation() defaults to rotation about the x axis for sliding doors with m_Rotate = (0,0,0),
	// so check for that special case. I don't want to change ToRotation() unless this proves
	// to be a general problem.

	idVec3 axis;
	if ( ( m_Rotate.yaw == 0 ) && ( m_Rotate.pitch == 0 ) && ( m_Rotate.roll == 0 ) )
	{
		axis.Zero();
	}
	else
	{
		axis = m_Rotate.ToRotation().GetVec(); 
	}
	return axis;
}

// grayman #2861 - get the closed origin

idVec3 CBinaryFrobMover::GetClosedOrigin()
{
	return m_ClosedOrigin;
}

// grayman #3643 - a copy of HandleElevatorTask::MoveToButton() modified for our purposes.
// Tells the AI where to stand when using the button or switch.

bool CBinaryFrobMover::GetSwitchGoal(idVec3 &goal, float &standOff, int relightHeightLow)
{
	standOff = AI_SIZE; // offset larger than the owner's size

	// This switch could be a button that translates horizontally or vertically,
	// or a lever that rotates.

	// Start with the assumption that it's a horizontally-translating button.
	// That should cover the majority of cases.

	idVec3 trans = spawnArgs.GetVector("translate", "0 0 0");
	if (trans.z != 0) // vertical movement?
	{
		idVec3 switchSize = GetPhysics()->GetBounds().GetSize();
		if (switchSize.y > switchSize.x)
		{
			trans = idVec3(1,0,0);
		}
		else
		{
			trans = idVec3(0,1,0);
		}
	}
	else if (trans.LengthFast() == 0)
	{
		// rotating switch

		idVec3 switchSize = GetPhysics()->GetBounds().GetSize();
		if (switchSize.x > switchSize.y)
		{
			trans = idVec3(1,0,0);
		}
		else
		{
			trans = idVec3(0,1,0);
		}
	}
	trans.NormalizeFast();

	const idVec3& switchOrigin = GetPhysics()->GetOrigin();

	// grayman #3648 - the following code has a problem sometimes finding
	// the AAS area of the goal spot when it's up by the switch. Push the
	// goal spot down to the floor and then try to find the AAS area.
	//
	// There needs to be LOS from the switch to the goal point. This should
	// prevent AI from operating a switch through a wall.

	trace_t result;

	bool pointFound = false;
	for ( int i = 0 ; i < 4 ; i++ )
	{
		switch (i)
		{
		case 0:
			break;
		case 1:
		case 3:
			trans *= -1;
			break;
		case 2:
			//const idVec3& gravity = owner->GetPhysics()->GetGravityNormal();
			const idVec3& gravity = this->GetPhysics()->GetGravityNormal(); // can we get gravity from the switch?
			trans = trans.Cross(gravity);
			break;
		}

		goal = switchOrigin - standOff * trans;
		if (!gameLocal.clip.TracePoint(result, switchOrigin, goal, MASK_OPAQUE, this))
		{
			// The switch can "see" the goal point.

			idVec3 bottomPoint = goal;
			bottomPoint.z -= 256;
			gameLocal.clip.TracePoint(result, goal, bottomPoint, MASK_OPAQUE, NULL); // trace down

			// success
			pointFound = true;
			break;
		}
	}

	if (!pointFound)
	{
		return false;
	}

	// If this switch controls a light that's being relit,
	// relightHeightLow will be > 0.

	if ( relightHeightLow > 0 )
	{
		float height = switchOrigin.z - result.endpos.z;

		// adjust standOff based on height of switch off the floor

		if (height < relightHeightLow) // low
		{
			// Adjust standoff and set new goal. Assume path to goal is still good.

			standOff += 16; // farther from goal
			goal = switchOrigin - standOff * trans;
		}
	}

	goal.z = result.endpos.z; // where we hit
	goal.z++; // move up slightly
	return true;
}

// grayman #3643 - copied from CFrobDoor
void CBinaryFrobMover::Event_ClearPlayerImmobilization(idEntity* player)
{
	if (!player->IsType(idPlayer::Type))
	{
		return;
	}

	// Release the immobilization imposed on the player by Lockpicking
	static_cast<idPlayer*>(player)->SetImmobilization("Lockpicking", 0);

	// stgatilov #4968: stop lockpicking if player's frob is broken
	// note: release does not look at lockpick type, so we pass garbage
	m_Lock->ProcessLockpickImpulse(EReleased, '-');
}

// grayman #3643 - copied from CFrobDoor
void CBinaryFrobMover::Event_Lock_StatusUpdate()
{
}

// grayman #3643 - copied from CFrobDoor
void CBinaryFrobMover::Event_Lock_OnLockPicked()
{
	// "Lock is picked" signal, unlock in master mode
	Unlock(true);
}

// grayman #4433
void CBinaryFrobMover::Event_GetFractionalPosition(void)
{
	// return a fraction from 0.00 (closed) to 1.00 (fully open)
	idThread::ReturnFloat(GetFractionalPosition());
}

void CBinaryFrobMover::SetMapOriginAxis(const idVec3 *newOrigin, const idMat3 *newAxis) {
	idVec3 oldOrigin = dest_position;
	idAngles oldAngles = dest_angles;
	//see also CBinaryFrobMover::PostSpawn and idMover::Spawn

	//save fraction/ratio of "openness"
	float frac = GetFractionalPosition();

	if (newOrigin) {
		dest_position = *newOrigin;
		m_ClosedOrigin = *newOrigin;
		m_OpenOrigin = m_ClosedOrigin + m_Translation;
	}

	if (newAxis) {
		idAngles newAngles = newAxis->ToAngles();
		dest_angles = newAngles;
		m_ClosedAngles = newAngles;
		m_ClosedAngles.Normalize180();
		m_OpenAngles = m_ClosedAngles + m_Rotate;
		m_OpenAngles.Normalize180();
	}

	//recompute m_ClosedPos, m_OpenPos, etc.
	ComputeAdditionalMembers();

	//restore fraction of "openness"
	SetFractionalPosition(frac, true);
}
