// DnAI_events.cpp
//

#include "../Gamelib/Game_local.h"

/*
===============
DnAI::Event_SetAnimation
===============
*/
void DnAI::Event_SetAnimation(const char* name, bool loop)
{
	int				animNum;
	int anim;
	const idAnim* newanim;

	animNum = animator.GetAnim(name);

	if (currentAnimation == name)
		return;

	if (!animNum) {
		gameLocal.Printf("Animation '%s' not found.\n", name);
		return;
	}

	anim = animNum;
	//starttime = gameLocal.time;
	//animtime = animator.AnimLength(anim);
	//headAnim = 0;

	currentAnimation = name;

	if (loop)
	{
		animator.CycleAnim(ANIMCHANNEL_ALL, anim, gameLocal.time, 0);
	}
	else
	{
		animator.PlayAnim(ANIMCHANNEL_ALL, anim, gameLocal.time, 0);
	}
	animator.RemoveOriginOffset(true);
}


/*
=====================
idAI::Event_TurnToward
=====================
*/
bool DnAI::Event_TurnToward(float yaw)
{
	ideal_yaw = idMath::AngleNormalize180(yaw);
	bool result = FacingIdeal();
	return result;
}

/*
=====================
DnAI::TurnToward
=====================
*/
bool DnAI::Event_TurnToward(const idVec3& pos)
{
	idVec3 dir;
	idVec3 local_dir;
	float lengthSqr;

	dir = physicsObj.GetOrigin() - pos;
	physicsObj.GetGravityAxis().ProjectVector(dir, local_dir);
	local_dir.z = 0.0f;
	ideal_yaw = local_dir.ToYaw();

	return true;
}

/*
=====================
DnAI::Event_UpdatePathToPosition
=====================
*/
void DnAI::Event_UpdatePathToPosition(idVec3 pos)
{
	if (pathWaypoints.Num() > 0)
	{
		float len = (pos - pathWaypoints[pathWaypoints.Num() - 1]).Length();
		if (len < 150)
		{
			len = (GetOrigin() - pathWaypoints[waypointId]).Length();
			if (len < 150)
			{
				waypointId++;

				if (waypointId >= pathWaypoints.Num())
					waypointId = pathWaypoints.Num() - 1;
			}

			MoveToPosition(pathWaypoints[waypointId]);

			return;
		}
	}

	waypointId = 0;
	pathWaypoints.Clear();
	if (!gameLocal.GetNavigation()->GetPathBetweenPoints(GetOrigin(), pos, pathWaypoints))
	{
		return;
	}

	MoveToPosition(pathWaypoints[0]);
}

/*
================
DnAI::Event_Hitscan
================
*/
void DnAI::Event_Hitscan(const char* damage_type, const idVec3& muzzleOrigin, const idVec3& dir, int num_hitscans, float spread, float power) {
	int areas[10];
	gameLocal.HitScan(damage_type, muzzleOrigin, dir, muzzleOrigin, this, false, 1.0f, NULL, areas, false);
}

/*
===============
DnAI::Event_FindNewTarget
===============
*/
idPlayer* DnAI::Event_FindNewTarget()
{
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	if (CanSee(localPlayer, false))
	{
		return localPlayer;
	}

	return nullptr;
}

/*
===============
DnAI::Event_ResetAnimation
===============
*/
void DnAI::Event_ResetAnimation() { 
	currentAnimation = ""; 
}

/*
===============
DnAI::Event_StopMove
===============
*/
void DnAI::Event_StopMove() { 
	StopMove(MOVE_STATUS_DONE); 
	physicsObj.SetLinearVelocity(vec3_zero); 
}
