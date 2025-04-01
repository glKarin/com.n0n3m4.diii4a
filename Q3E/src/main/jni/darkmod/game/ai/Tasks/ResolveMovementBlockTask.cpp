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



#include "ResolveMovementBlockTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

ResolveMovementBlockTask::ResolveMovementBlockTask() :
	_blockingEnt(NULL),
	_preTaskContents(-1)
{}

ResolveMovementBlockTask::ResolveMovementBlockTask(idEntity* blockingEnt) :
	_blockingEnt(blockingEnt),
	_preTaskContents(-1)
{}

// Get the name of this task
const idStr& ResolveMovementBlockTask::GetName() const
{
	static idStr _name(TASK_RESOLVE_MOVEMENT_BLOCK);
	return _name;
}

/* grayman #4238 - not used
bool ResolveMovementBlockTask::IsSolid() // grayman #2345
{
	return (_preTaskContents == -1);
}
*/

void ResolveMovementBlockTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	//owner->GetMemory().resolvingMovementBlock = true; // grayman #4077 - moved to before this task is pushed,
														// to prevent starting this task twice

	if (_blockingEnt == NULL)
	{
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s cannot resolve a NULL blocking entity.\r", owner->name.c_str());
		owner->PushMove(); // grayman #2706 - save movement state, because it gets popped in OnFinish()
		subsystem.FinishTask();
		return; // grayman #3670
	}

	// grayman #3725 - no task if _blockingEnt is an AI and is too far away
	if (_blockingEnt->IsType(idAI::Type))
	{
		idVec3 dist = _blockingEnt->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
		if (dist.LengthSqr() > Square(60))
		{
			owner->PushMove();
			subsystem.FinishTask();
			return;
		}
	}

	// Get the direction we're pushing against
	_initialAngles = owner->viewAxis.ToAngles();

	// Set the TTL for this task
	_endTime = gameLocal.time + 20000; // 20 seconds

	// grayman #3725
	_turning = false;

	// Save the movement state
	owner->PushMove();

	if (_blockingEnt->IsType(idAI::Type))
	{
		//DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s starting to resolve blocking AI: %s\r", owner->name.c_str(), _blockingEnt->name.c_str());
		InitBlockingAI(owner, subsystem);
	}
	// grayman #4077 - also add a check for a door here, in case the AI is bumping
	// up against a door that was opened in his face before he could latch onto it
	// for door handling (St. Lucia)
	else if ( _blockingEnt->IsType(idStaticEntity::Type) || _blockingEnt->IsType(CFrobDoor::Type) )
	{
		//DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s starting to resolve static blocking entity: %s\r", owner->name.c_str(), _blockingEnt->name.c_str());
		InitBlockingStatic(owner, subsystem);
	}
	else
	{
		// Unknown entity type, exit task
		//DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s cannot resolve blocking entity: %s\r", owner->name.c_str(), _blockingEnt->name.c_str());
		subsystem.FinishTask(); 
	}
}

void ResolveMovementBlockTask::InitBlockingAI(idAI* owner, Subsystem& subsystem)
{
	// grayman #4238 - stepping to the left or right doesn't work, because the block
	//   is still happening, and the AI is told to stop its move further along in the
	//   same frame. Leaving it in for now, though.

	// grayman #2345 - The old method was to move 10 units to the right, but that could make things
	// worse, by pushing you more into the path of your blocker.

	// Old method:

//		idVec3 ownerRight, ownerForward;
//		_initialAngles.ToVectors(&ownerForward, &ownerRight);
//		owner->MoveToPosition(owner->GetPhysics()->GetOrigin() + ownerRight*10, 5);

	// New method:

	// Instead, draw a line from you to your blocker, and find their forward vector.
	// If their forward vector is on the left of the connecting line, you go to the right.
	// If their forward vector is on the right of the connecting line, you go to the left.
	// These are not right and left based on your own forward vector. They're relevant to
	// the line between you and your blocker.

	idVec3 blockingOrigin = _blockingEnt->GetPhysics()->GetOrigin();
	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
	idVec3 delta = (blockingOrigin - ownerOrigin);
	delta.z = 0; // ignore vertical component
	delta.Normalize(); // set length = 1

	// grayman #3857 - use correct forward angle for actors
	idVec3 blockingForward;
	if (_blockingEnt->IsType(idActor::Type))
	{
		blockingForward = static_cast<idActor*>(_blockingEnt)->viewAxis.ToAngles().ToForward();
	}
	else
	{
		blockingForward = _blockingEnt->GetPhysics()->GetAxis().ToAngles().ToForward();
	}

	float cx = blockingOrigin.x + blockingForward.x;
	float cy = blockingOrigin.y + blockingForward.y;
	float result = (blockingOrigin.x - ownerOrigin.x) * (cy - ownerOrigin.y) - (blockingOrigin.y - ownerOrigin.y)*(cx - ownerOrigin.x);
	idVec3 dest = ownerOrigin;
	if (result > 0.0f)
	{
		// head to the right of the blocking entity
		dest.x += RESOLVE_MOVE_DIST*delta.y;
		dest.y -= RESOLVE_MOVE_DIST*delta.x;
	}
	else
	{
		// head to the left of the blocking entity
		dest.x -= RESOLVE_MOVE_DIST*delta.y;
		dest.y += RESOLVE_MOVE_DIST*delta.x;
	}

	owner->MoveToPosition(dest, 5);
	_turning = true; // grayman #4238 - you'll need to turn to walk around the blocking entity
	owner->movementSubsystem->SetBlockedState(ai::MovementSubsystem::EResolvingBlock); // grayman #2706 - stay in EResolvingBlock
}

void ResolveMovementBlockTask::InitBlockingStatic(idAI* owner, Subsystem& subsystem)
{
	// Get the bounds of the blocking entity and see if there is a way around it
	idPhysics* blockingPhys = _blockingEnt->GetPhysics();
	idBounds blockBounds = blockingPhys->GetAbsBounds();

	// grayman #2356 - special case of being stuck inside a func_static, i.e. a rat under a bed that's not monster_clipped

	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
	if (blockBounds.ContainsPoint(ownerOrigin))
	{
		// Try to get out, otherwise you're stuck until your surroundings change.

		if (owner->movementSubsystem->AttemptToExtricate())
		{
			owner->movementSubsystem->SetBlockedState(ai::MovementSubsystem::EResolvingBlock); // grayman #2706 - stay in EResolvingBlock
			return;
		}

		// oh boy, you're really stuck

		owner->StopMove(MOVE_STATUS_DONE);
		owner->AI_BLOCKED = false;
		owner->AI_DEST_UNREACHABLE = false;
		subsystem.FinishTask();
		return;
	}

	blockBounds[0].z = blockBounds[1].z = 0;

	// Check if there is space to the right of the obstacle
	idBounds bounds = owner->GetPhysics()->GetBounds();

	if (owner->GetAAS() == NULL)
	{
		return;
	}

	// angua: move the bottom of the bounds up a bit, to avoid finding small objects on the ground that are "in the way"
	// grayman #2684 - except for AI whose bounding box height is less than maxStepHeight, otherwise applying the bump up
	// causes the clipmodel to be "upside-down", which isn't good. In that case, give the bottom a bump up equal to half
	// of the clipmodel's height so it at least gets a small bump.
	float ht = owner->GetAAS()->GetSettings()->maxStepHeight;
	if (bounds[0].z + ht < bounds[1].z)
	{
		bounds[0].z += ht;
	}
	else
	{
		bounds[0].z += (bounds[1].z - bounds[0].z)/2.0;
	}

	// Set all attachments to nonsolid, temporarily
	owner->SaveAttachmentContents();
	owner->SetAttachmentContents(0);

	// check if there is a way around
	idTraceModel trm(bounds);
	idClipModel clip(trm);

	idVec3 ownerRight, ownerForward;
	_initialAngles.ToVectors(&ownerForward, &ownerRight);

	// Take a point to the right
	idVec3 testPoint = blockingPhys->GetOrigin();
	testPoint.z = ownerOrigin.z;

	// Move one AAS bounding box size outwards from the model
	float blockBoundsWidth = blockBounds.GetRadius(blockBounds.GetCenter());

	idBounds aasBounds = owner->GetAAS()->GetSettings()->boundingBoxes[0];
	aasBounds[0].z = aasBounds[1].z = 0;

	float aasBoundsWidth = aasBounds.GetRadius();

	testPoint += ownerRight * (blockBoundsWidth + aasBoundsWidth);

	int contents = gameLocal.clip.Contents(testPoint, &clip, mat3_identity, CONTENTS_SOLID, owner);

	if (cv_ai_debug_blocked.GetBool())
	{
		idVec3 temp = blockingPhys->GetOrigin();
		temp.z = ownerOrigin.z;

		gameRenderWorld->DebugArrow(colorWhite, temp, temp + idVec3(0,0,10), 1, 1000);

		temp += ownerRight * (blockBoundsWidth + aasBoundsWidth + 5);
		gameRenderWorld->DebugArrow(colorLtGrey, temp, temp + idVec3(0,0,10), 1, 1000);

		gameRenderWorld->DebugBounds(contents ? colorRed : colorGreen, bounds, testPoint, 1000);
	}

	// grayman #2345 - the point tested might be in the VOID, so let's check for a valid AAS area

	int areaNum = owner->PointReachableAreaNum(testPoint);

	if ((contents != 0) || (areaNum == 0))
	{
		// Right side is blocked, look at the left.

		testPoint = blockingPhys->GetOrigin();
		testPoint.z = ownerOrigin.z;
		testPoint -= ownerRight * (blockBoundsWidth + aasBoundsWidth + 5);

		contents = gameLocal.clip.Contents(testPoint, &clip, mat3_identity, CONTENTS_SOLID, owner);

		if (cv_ai_debug_blocked.GetBool())
		{
			gameRenderWorld->DebugBounds(contents ? colorRed : colorGreen, bounds, testPoint, 1000);
		}

		areaNum = owner->PointReachableAreaNum(testPoint); // testPoint must not be in the VOID
		
		if ((contents != 0) || (areaNum == 0))
		{
			// Neither left nor right has free space

			// grayman #2345 - before declaring failure, let's try extrication

			owner->RestoreAttachmentContents(); // AttemptToExtricate() will do an attachment save/restore, so we must restore here first
			owner->movementSubsystem->AttemptToExtricate();
		}
		else
		{
			// Move to left position
			owner->MoveToPosition(testPoint);
			owner->RestoreAttachmentContents();
		}
	}
	else
	{
		// Move to right position
		owner->MoveToPosition(testPoint);
		owner->RestoreAttachmentContents();
	}
	owner->movementSubsystem->SetBlockedState(ai::MovementSubsystem::EResolvingBlock); // grayman #2706 - stay in EResolvingBlock
}

bool ResolveMovementBlockTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ResolveMovementBlockTask performing.\r");

	if (gameLocal.time > _endTime)
	{
		// Timeout
		return true;
	}

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	if (_blockingEnt->IsType(idAI::Type))
	{
		return PerformBlockingAI(owner);
	}

	// grayman #4077 - also add a check for a door here, in case the AI is bumping
	// up against a door that was opened in his face before he could latch onto it
	// for door handling (St. Lucia)
	if (_blockingEnt->IsType(idStaticEntity::Type) || _blockingEnt->IsType(CFrobDoor::Type))
	{
		return PerformBlockingStatic(owner);
	}
	
	return false; // not finished yet
}

bool ResolveMovementBlockTask::Room2Pass(idAI* owner) // grayman #2345
{
	bool result = false;

	// Test the space around you. If the blocking AI is heading generally north/south,
	// check to your east and west. If the blocking AI is heading generally east/west,
	// check to your north and south. If there's enough room for the blocking AI to pass
	// through one of the two spaces you check, then return TRUE. If not, return FALSE.
	// The calling routine will then make you non-solid if that's necessary to let the
	// blocking AI pass by.

	// Find out where the blocking AI is headed.

	if (!_blockingEnt->IsType(idAI::Type))
	{
		return result; // not an AI
	}
	
	idAI* blockingAI = static_cast<idAI*>(_blockingEnt);
	idVec3 blockingGoalPos = blockingAI->GetMoveDest();
	idVec3 blockingOrigin = blockingAI->GetPhysics()->GetOrigin();
	idVec3 dir = blockingGoalPos - blockingOrigin;
	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
	idVec3 end1 = ownerOrigin;
	idVec3 end2 = ownerOrigin;

	if (abs(dir.y) > abs(dir.x))
	{
		// blocking AI is headed n/s, so test to your e/w

		end1.x += 48;
		end2.x -= 48;
	}
	else
	{
		// blocking AI is headed e/w, so test to your n/s

		end1.y += 48;
		end2.y -= 48;
	}

	trace_t traceResult;
	idBounds bnds = owner->GetPhysics()->GetBounds();
	gameLocal.clip.TraceBounds(traceResult, ownerOrigin, end1, bnds, CONTENTS_SOLID|CONTENTS_CORPSE, owner);
	if (traceResult.fraction >= 1.0)
	{
		result = true;
	}
	else
	{
		gameLocal.clip.TraceBounds(traceResult, ownerOrigin, end2, bnds, CONTENTS_SOLID|CONTENTS_CORPSE, owner);
		if (traceResult.fraction >= 1.0)
		{
			result = true;
		}
	}

	return result;
}

void ResolveMovementBlockTask::BecomeNonSolid(idAI* owner) // grayman #2345
{
	bool solid = false;
	_preTaskContents = owner->GetPhysics()->GetContents();
	owner->GetPhysics()->SetContents(0);

	// Set all attachments to nonsolid, temporarily

	owner->SaveAttachmentContents();
	owner->SetAttachmentContents(0);
	owner->movementSubsystem->SetWaiting(solid);
}

bool ResolveMovementBlockTask::PerformBlockingAI(idAI* owner)
{
	idVec3 right, forward;

	idVec3 dist = _blockingEnt->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();

	// grayman #4238 - if _blockingEnt is still conscious and is farther
	// than 60 units away, end the task, regardless of what state you're in
	idAI* beAI = static_cast<idAI*>(_blockingEnt);
	if ( (dist.LengthSqr() > Square(60) ) || beAI->AI_DEAD || beAI->AI_KNOCKEDOUT )
	{
		// Return to your initial facing angle. This will
		// only matter if you were initially standing. If you were moving,
		// ending this task will return you to that, overriding this turn.

		_initialAngles.ToVectors(&forward, &right);
		owner->TurnToward(owner->GetPhysics()->GetOrigin() + forward);
		return true;
	}

	// grayman #3725 - if still walking and not turning, but not getting
	// anywhere, stop moving, to allow yourself to turn and become non-solid
	float traveledPrev = owner->movementSubsystem->GetPrevTraveled(false).LengthFast();
	if (!owner->AI_MOVE_DONE && !_turning && (traveledPrev < 0.1)) // grayman #2345
	{
		owner->StopMove(MOVE_STATUS_DONE);
	}
	
	if (owner->AI_MOVE_DONE && !_turning && !owner->movementSubsystem->IsWaiting()) // grayman #2345 - if already waiting, no need to do this section
	{
		// grayman #3725 - turn perpendicular to the direction
		// _blockingEnt is looking
		idAngles angles;
		if (_blockingEnt->IsType(idAI::Type))
		{
			angles = beAI->viewAxis.ToAngles();
		}
		else
		{
			angles = _initialAngles;
		}
		angles.ToVectors(&forward, &right);
		owner->StopMove(MOVE_STATUS_WAITING);
		owner->TurnToward(owner->GetPhysics()->GetOrigin() + right);
		_turning = true; // grayman #3725
	}

	if (_turning) // grayman #3725
	{
		if (!owner->FacingIdeal())
		{
			return false;
		}

		_turning = false;

		if (_preTaskContents == -1)
		{
			// grayman #2345 - don't become non-solid if your alert index is > ERelaxed. This is because
			// AI tend to bunch together when agitated, and it doesn't look good if one goes non-solid
			// and the others repeatedly walk through it.

			// If there's no room to get around you, become non-solid

			// grayman - With coordinated searching, AI are less likely to be bunching up,
			// so remove the requirement that the AI be in ERelaxed to become non-solid

			if (/*(owner->AI_AlertIndex == ERelaxed) && */!Room2Pass(owner))
			{
				BecomeNonSolid(owner);
			}
			else
			{
				bool solid = true;
				owner->movementSubsystem->SetWaiting(solid);
			}
		}
	}

	if (owner->movementSubsystem->IsWaiting()) // grayman #2345
	{
		// We are waiting for the other AI to pass by
		idVec3 dist = _blockingEnt->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();

		if (dist.LengthSqr() > Square(60))
		{
			// other AI has passed by, end the task

			// Be sure no one else is touching you when you want to return to being solid.
			// If there is, remain non-solid and let them pass by before you return to solid.
			// (This might not matter, but let's be safe.)

			idClip_ClipModelList clipModels;
			idPhysics *phys = owner->GetPhysics();
			int num = gameLocal.clip.ClipModelsTouchingBounds( phys->GetAbsBounds(), phys->GetClipMask(), clipModels );
			bool foundNeighbor = false;
			for ( int i = 0 ; i < num ; i++ )
			{
				idClipModel *cm = clipModels[ i ];

				// don't check render entities
				if ( cm->IsRenderModel() )
				{
					continue;
				}

				idEntity *hit = cm->GetEntity();
				if ( hit == owner )
				{
					continue;
				}

				if ( !phys->ClipContents( cm ) )
				{
					continue;
				}

				// touching
				if ( hit->IsType( idActor::Type ) )
				{
					_blockingEnt = hit;
					foundNeighbor = true;
					break;
				}
			}

			if (!foundNeighbor)
			{
				// grayman #3725 - Return to your initial facing angle. This will
				// only matter if you were initially standing. If you were moving,
				// ending this task will return you to that, overriding this turn.

				_initialAngles.ToVectors(&forward, &right);
				owner->TurnToward(owner->GetPhysics()->GetOrigin() + forward);
				return true;
			}
		}
		
		//if (_blockingEnt->IsType(idAI::Type))
		//{
			// grayman #2345 - check to see if the other AI is standing still.
			// If they are, end the task.
			// grayman #4238 - only end the task if the blocking entity is at least
			// 2xsize away.

			if (beAI && !beAI->AI_FORWARD)
			{
				// Check distance to blocking entity
				if ( dist.LengthSqr() > Square(32) )
				{
					// grayman #3725 - turn back to your initial facing angle
					_initialAngles.ToVectors(&forward, &right);
					owner->TurnToward(owner->GetPhysics()->GetOrigin() + forward);
					return true; // end the task
				}

				// grayman #4238 - move away from standing blocking entity
				float needToMove = 33 - dist.LengthFast();
				dist.NormalizeFast();
				owner->MoveToPosition(owner->GetPhysics()->GetOrigin() + needToMove*dist);
				//return true; // grayman #4238 - end the task
			}

			// If we're EWaitingSolid, change to EWaitingNonSolid if the other AI is barely moving.
			// grayman #2422 - but not if the other AI is searching

			if ( !beAI->IsSearching() )
			{
				if (owner->movementSubsystem->IsWaitingSolid())
				{
					float traveledPrev = beAI->movementSubsystem->GetPrevTraveled(false).LengthFast(); // grayman #3647
					if (traveledPrev < 0.1) // grayman #2345
					{
						BecomeNonSolid(owner);
					} 
				}
			}
		//}

		if (ai_showObstacleAvoidance.GetBool())
		{
			// waiting AI is watching blocking AI
			gameRenderWorld->DebugLine(colorPink,owner->GetPhysics()->GetOrigin() + idVec3(0,0,72),_blockingEnt->GetPhysics()->GetOrigin() + idVec3(0,0,64),100);
		}
	}

	return false;
}

bool ResolveMovementBlockTask::PerformBlockingStatic(idAI* owner) // grayman #2345 - entirely replaced
{
	if (owner->AI_MOVE_DONE)
	{
		return true;
	}

	// If you're not getting anywhere, try extricating yourself.

	if (owner->movementSubsystem->GetPrevTraveled(false).LengthFast() < 0.1) // grayman #3647
	{
		// grayman #3119 - if searching, and you're going nowhere,
		// restart the search, in the hope that you'll be sent
		// somewhere new

		// grayman #3857 - TODO: do we want to restart the search, or just leave the search and rejoin?

		if ( owner->IsSearching() )
		{
			owner->GetMemory().restartSearchForHidingSpots = true;
		}
		else
		{
			owner->movementSubsystem->AttemptToExtricate();
		}
		owner->movementSubsystem->SetBlockedState(ai::MovementSubsystem::EResolvingBlock); // grayman #2706 - stay in EResolvingBlock
	}

	return false;
}

void ResolveMovementBlockTask::OnFinish(idAI* owner)
{
	owner->GetMemory().resolvingMovementBlock = false;

	// grayman #4238 - check content instead of waiting state
	//if (owner->movementSubsystem->IsWaiting()) // grayman #2345
	//{
		if (_preTaskContents != -1)
		{
			owner->GetPhysics()->SetContents(_preTaskContents);
			//_preTaskContents = -1;

			// Restore attachment contents again

			owner->RestoreAttachmentContents();
		}

		//_blockingEnt = NULL; // forget the other entity
	//}

	owner->movementSubsystem->SetBlockedState(ai::MovementSubsystem::ENotBlocked); // grayman #2345
	owner->PopMove();
}

// Save/Restore methods
void ResolveMovementBlockTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteObject(_blockingEnt);
	savefile->WriteAngles(_initialAngles);
	savefile->WriteInt(_preTaskContents);
	savefile->WriteBool(_turning); // grayman #3725
	savefile->WriteInt(_endTime);
}

void ResolveMovementBlockTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_blockingEnt));
	savefile->ReadAngles(_initialAngles);
	savefile->ReadInt(_preTaskContents);
	savefile->ReadBool(_turning); // grayman #3725
	savefile->ReadInt(_endTime);
}

ResolveMovementBlockTaskPtr ResolveMovementBlockTask::CreateInstance()
{
	return ResolveMovementBlockTaskPtr(new ResolveMovementBlockTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar resolveMovementBlockTaskRegistrar(
	TASK_RESOLVE_MOVEMENT_BLOCK, // Task Name
	TaskLibrary::CreateInstanceFunc(&ResolveMovementBlockTask::CreateInstance) // Instance creation callback
);

} // namespace ai
