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



#include "../Memory.h"
#include "PathCornerTask.h"
#include "../Library.h"
#include "../../MultiStateMover.h" // grayman #3647

namespace ai
{

PathCornerTask::PathCornerTask() :
	PathTask(),
	_moveInitiated(false),
	_movePaused(false), // grayman #3647
	_lastPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_lastFrameNum(-1),
	_usePathPrediction(false)
{}

PathCornerTask::PathCornerTask(idPathCorner* path) :
	PathTask(path),
	_moveInitiated(false),
	_movePaused(false), // grayman #3647
	_lastPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_lastFrameNum(-1),
	_usePathPrediction(false)
{
	_path = path;
}

// Get the name of this task
const idStr& PathCornerTask::GetName() const
{
	static idStr _name(TASK_PATH_CORNER);
	return _name;
}

void PathCornerTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	PathTask::Init(owner, subsystem);

	_lastPosition = owner->GetPhysics()->GetOrigin();
	_lastFrameNum = gameLocal.framenum;

	// Check the "run" spawnarg of this path entity
	owner->AI_RUN = (_path.GetEntity()->spawnArgs.GetBool("run", "0"));

	idPathCorner* nextPath = owner->GetMemory().nextPath.GetEntity();

	// Allow path prediction only if the next path is an actual path corner and no accuracy is set on this one
	if (_accuracy == -1 && nextPath != NULL && idStr::Icmp(nextPath->spawnArgs.GetString("classname"), "path_corner") == 0)
	{
		_usePathPrediction = true;
	}
}

bool PathCornerTask::Perform(Subsystem& subsystem)
{
	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// grayman #2345 - if you've timed out of a door queue, go back to your
	// previous path_corner

	if (owner->m_leftQueue)
	{
		owner->m_leftQueue = false;
		Memory& memory = owner->GetMemory();
		idPathCorner* tempPath = memory.lastPath.GetEntity();
		if (tempPath != NULL)
		{
			memory.lastPath = path;
			_path = tempPath;
			path = tempPath;
			memory.currentPath = path;
			memory.nextPath = idPathCorner::RandomPath(path, NULL, owner);
			owner->StopMove(MOVE_STATUS_DONE); // lets the new pathing take over
		}
	}

	if (_moveInitiated)
	{
		const idVec3& ownerOrigin = owner->GetPhysics()->GetOrigin();

		if (owner->AI_MOVE_DONE)
		{
			if (owner->ReachedPos(path->GetPhysics()->GetOrigin(), MOVE_TO_POSITION))
			{
				// Trigger path targets, now that we've reached the corner
				// grayman #3670 - need to keep the owner->Activate() calls to not break
				// existing maps, but the intent was path->Activate().
				owner->ActivateTargets(owner);
				path->ActivateTargets(owner);

				// NextPath();

				// Move is done, fall back to PatrolTask
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Move is done.\r");

				// grayman #3989 - Save the ideal sitting/sleeping location for patrolling to return to after
				// an interruption. Though saved for all path_corner nodes and the original AI location when
				// the AI starts with a "sitting" or "sleeping" spawnarg, it is only used by
				// Event_GetVectorToIdealOrigin(), which is called by the sitting/sleeping animations.

				owner->GetMemory().returnSitPosition = path->GetPhysics()->GetOrigin();

				// grayman #3755 - reset time this door can be used again

				Memory& memory = owner->GetMemory();
				CFrobDoor* frobDoor = memory.lastDoorHandled.GetEntity();
				if (frobDoor != NULL)
				{
					memory.GetDoorInfo(frobDoor).timeCanUseAgain = gameLocal.time; // grayman #3755 - reset timeout
				}

				return true; // finish this task
			}
			else
			{
				owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy);
			}
		}
		else if (_usePathPrediction) 
		{
			// Move not done yet. Try to perform a prediction whether we will hit the path corner
			// next round. This is valid, as no accuracy is set on the current path.
			
			idVec3 moveDeltaVec = ownerOrigin - _lastPosition;
			float moveDelta = moveDeltaVec.NormalizeFast();

			// grayman #2414 - start of new prediction code

			if (moveDelta > 0)
			{
				idVec3 toPath = path->GetPhysics()->GetOrigin() - ownerOrigin;

				// grayman #3029 - Ignore path_corners that are out of reach.
				// This prevents 'getting near' path_corners that are too far away vertically.
				// Originally added as a fix for #2717 elsewhere, but not here.

				idBounds bounds = owner->GetPhysics()->GetBounds();

				if ( ( abs(toPath.z) < (bounds[1][2] + 0.4*owner->GetReachTolerance()) ) ) // don't look so far up or down
				{
					toPath.z = 0; // ignore vertical component
					float distToPath = toPath.NormalizeFast();

					// If the move direction and the distance vector to the path are pointing in roughly the same direction,
					// the prediction will be rather accurate.

					if (toPath * moveDeltaVec > 0.7f)
					{
						bool turnNow = false; // whether it's time to make the turn

						// will we overshoot the path_corner within the next two checks?

						if (distToPath <= PATH_PREDICTION_MOVES*moveDelta) // quick check for overshooting
						{
							turnNow = true;
						}
						else
						{
							int frameDelta = gameLocal.framenum - _lastFrameNum;
							float factor = PATH_PREDICTION_MOVES + PATH_PREDICTION_CONSTANT/static_cast<float>(frameDelta);
							if (distToPath <= factor*moveDelta) // consider the size of the AI's bounding box if we're w/in a certain range
							{
								// Virtually translate the path_corner position back to the origin and
								// see if ReachedPos's box check would succeed.

								turnNow = owner->ReachedPosAABBCheck(path->GetPhysics()->GetOrigin() - toPath*factor*moveDelta);
							}
						}

						if (turnNow)
						{
							// Trigger path targets, now that we've almost reached the corner
							// grayman #3670 - need to keep the owner->Activate() calls to not break
							// existing maps, but the intent was path->Activate().
							owner->ActivateTargets(owner);
							path->ActivateTargets(owner);

							// NextPath();

							// Move is done, fall back to PatrolTask
							DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathCornerTask ending prematurely.\r");

							// grayman #5156 - need to include this missing line
							// grayman #3989 - Save the ideal sitting/sleeping location for patrolling to return to after
							// an interruption. Though saved for all path_corner nodes and the original AI location when
							// the AI starts with a "sitting" or "sleeping" spawnarg, it is only used by
							// Event_GetVectorToIdealOrigin(), which is called by the sitting/sleeping animation scripts.
							owner->GetMemory().returnSitPosition = path->GetPhysics()->GetOrigin();

							// End this task, let the next patrol/pathcorner task take up its work before
							// the AI code is actually reaching its position and issuing StopMove
							return true;
						}
					}
				}
			}
			else 
			{
				// No movement - this can happen right at the first execution or when blocked
				// Blocks are handled by the movement subsystem, so ignore this case
			}
		}

		_lastPosition = ownerOrigin;
		_lastFrameNum = gameLocal.framenum;

		if (owner->AI_DEST_UNREACHABLE)
		{
			// Unreachable, fall back to PatrolTask
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Destination is unreachable, ending PathCornerTask, falling back to PatrolTask.\r");
			idVec3 prevMove = owner->movementSubsystem->GetPrevTraveled(true); // grayman #3647

			// NextPath();
			return true; // finish this task
		}

		// Move...
	}
	else if (_movePaused)// grayman #3647
	{
		// wait for vertical movement to stop
		idVec3 prevMove = owner->movementSubsystem->GetPrevTraveled(true); // grayman #3647
		if (prevMove.z == 0.0)
		{
			_movePaused = false; // turn this off and try again
		}
	}
	else
	{
		// MoveToPosition() not yet called, do it now
		owner->StopMove(MOVE_STATUS_DEST_NOT_FOUND);

		// grayman #3647 - test return value of MoveToPosition(). This is the point
		// where we first know that a destination is unreachable. In the case of being
		// caught on an elevator between floors, no starting AAS area is available, so
		// we have to figure out how to detect that situation and deal with it. If the
		// elevator is moving, we should wait for it to reach the next floor.
		// If the elevator isn't moving, we might have to force the AI to get it started
		// again w/o using pathfinding. Currently, there is no elevator task running.
		if (owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy))
		{
			_moveInitiated = true;
		}
		else
		{
			idVec3 prevMove = owner->movementSubsystem->GetPrevTraveled(true); // grayman #3647
			if (prevMove.z != 0.0)
			{
				// Vertical movement suggests being on an elevator. Trace down and see
				// if you hit one.
				idVec3 startPoint = owner->GetPhysics()->GetOrigin();
				idVec3 bottomPoint = startPoint;
				bottomPoint.z -= 10;

				trace_t result;
				if ( gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, owner) )
				{
					// Found something ...

					idEntity* ent = gameLocal.entities[result.c.entityNum];
					if (ent->IsType(CMultiStateMover::Type))
					{
						// ... and it's an elevator. Pause the task until vertical movement
						// stops. You should then be at an elevator station with an AAS area.

						_movePaused = true;
					}
				}
			}
			else // no vertical movement
			{
				_moveInitiated = true; // Continue previous bad behavior
			}
		}
	}

	return false; // not finished yet
}


// Save/Restore methods
void PathCornerTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteBool(_moveInitiated);
	savefile->WriteBool(_movePaused); // grayman #3647
	savefile->WriteVec3(_lastPosition);
	savefile->WriteInt(_lastFrameNum);
	savefile->WriteBool(_usePathPrediction);
}

void PathCornerTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadBool(_moveInitiated);
	savefile->ReadBool(_movePaused); // grayman #3647
	savefile->ReadVec3(_lastPosition);
	savefile->ReadInt(_lastFrameNum);
	savefile->ReadBool(_usePathPrediction);
}

PathCornerTaskPtr PathCornerTask::CreateInstance()
{
	return PathCornerTaskPtr(new PathCornerTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathCornerTaskRegistrar(
	TASK_PATH_CORNER, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathCornerTask::CreateInstance) // Instance creation callback
);

} // namespace ai
