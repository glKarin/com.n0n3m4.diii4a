// Monster_Pigcop.cpp
//


#include "../Gamelib/Game_local.h"

#define PIGCOP_FIRE_DISTANCE			500
#define PIGCOP_FORCE_FIREDISTANCE		200

CLASS_DECLARATION(DnAI, DnPigcop)
END_CLASS

/*
==============
DnPigcop::state_Begin
==============
*/
stateResult_t DnPigcop::state_Begin(stateParms_t* parms)
{
	Event_SetAnimation("idle", false);

	Event_SetState("state_Idle");

	shotgunMeshComponent.Init(this, renderModelManager->FindModel("models/weapons/w_shotgun/w_shotgun.md3"));
	shotgunMeshComponent.BindToJoint("HAND_L");
	RegisterComponent(&shotgunMeshComponent);

	pig_roam1 = declManager->FindSound("pig_roam1", false);
	pig_roam2 = declManager->FindSound("pig_roam2", false);
	pig_roam3 = declManager->FindSound("pig_roam3", false);
	pig_awake = declManager->FindSound("pig_awake", false);
	fire_sound = declManager->FindSound("pig_fire_sound", false);
	death_sound = declManager->FindSound("pig_death", false);

	return SRESULT_DONE;
}

/*
==============
DnPigcop::state_BeginDeath
==============
*/
stateResult_t DnPigcop::state_BeginDeath(stateParms_t* parms)
{
	Event_SetState("state_Killed");

	Event_StopMove();

	Event_SetAnimation("death", false);

	StartSoundShader(death_sound, SND_CHANNEL_ANY, 0, false, nullptr);

	shotgunMeshComponent.Destroy();

	idEntity* ent = gameLocal.Spawn("item_shotgun");
	DnItemShotgun*weap = ent->Cast<DnItemShotgun>();

	idVec3 itemStartOrigin = GetOrigin() + idVec3(0, 0, 35);

	weap->orgOrigin = itemStartOrigin;
	weap->SetOrigin(itemStartOrigin);

	return SRESULT_DONE;
}

/*
==============
DnPigcop::state_Killed
==============
*/
stateResult_t DnPigcop::state_Killed(stateParms_t* parms)
{
	return SRESULT_WAIT; // Were dead so do nothing.
}

/*
==============
DnPigcop::state_ShootEnemy
==============
*/
stateResult_t DnPigcop::state_ShootEnemy(stateParms_t* parms)
{
	// If we are firing, don't make any new decisions until its done.
	if ((animator.IsAnimating(gameLocal.time) || CurrentlyPlayingSound()) && GetCurrentAnimation() == "fire")
	{
		Event_TurnToward(target->GetOrigin());
		animator.RemoveOriginOffset(true);
		return SRESULT_WAIT;
	}

	float distToEnemy = 0.0f;
	distToEnemy = (target->GetOrigin() - GetOrigin()).Length();

	if (distToEnemy < PIGCOP_FIRE_DISTANCE + 25 || AI_PAIN)
	{
		if (!isTargetVisible)
		{
			Event_SetState("state_ApproachingEnemy");
			return SRESULT_DONE;
		}

		Event_TurnToward(target->GetOrigin());
		Event_ResetAnimation();
		Event_SetAnimation("fire", false);
		StartSoundShader(fire_sound, SND_CHANNEL_ANY, 0, false, nullptr);

		idVec3 muzzleOrigin = GetOrigin() + idVec3(0, 0, 40);
		idVec3 muzzleDir = muzzleOrigin - (target->GetOrigin() + target->GetVisualOffset());

		muzzleDir.Normalize();
		Event_Hitscan("damage_pigcop", muzzleOrigin, -muzzleDir, 1, 1, 20);

		return SRESULT_WAIT;
	}
	else
	{
		Event_SetState("state_ApproachingEnemy");
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

/*
==============
DnPigcop::state_ApproachingEnemy
==============
*/
stateResult_t DnPigcop::state_ApproachingEnemy(stateParms_t* parms)
{
	float distToEnemy = 0.0f;

	distToEnemy = (target->GetOrigin() - GetOrigin()).Length();

	if (distToEnemy > PIGCOP_FIRE_DISTANCE || !isTargetVisible || (CurrentlyPlayingSound() && distToEnemy >= PIGCOP_FORCE_FIREDISTANCE))
	{
		Event_UpdatePathToPosition(target->GetOrigin());
		Event_SetAnimation("walk", true);
	}
	else
	{
		Event_StopMove();
		Event_SetState("state_ShootEnemy");
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

/*
==============
DnPigcop::state_Idle
==============
*/
stateResult_t DnPigcop::state_Idle(stateParms_t* parms)
{
	switch (parms->stage)
	{
		case PIGCOP_IDLE_WAITINGTPLAYER:
			target = Event_FindNewTarget();

			if (target != nullptr)
			{
				targetLastSeenLocation = target->GetOrigin();
				isTargetVisible = true;

				Event_SetAnimation("roar", false);
				StartSoundShader(pig_awake, SND_CHANNEL_ANY, 0, false, nullptr);
				parms->stage = PIGCOP_IDLE_ROAR;
			}
			else
			{
				if (!CurrentlyPlayingSound())
				{
					if (dnRand.ifrnd(1))
					{
						if (dnRand.ifrnd(32))
						{
							StartSoundShader(pig_roam1, SND_CHANNEL_ANY, 0, false, nullptr);
						}
						else
						{
							if (dnRand.ifrnd(64))
							{
								StartSoundShader(pig_roam2, SND_CHANNEL_ANY, 0, false, nullptr);
							}
							else
							{
								StartSoundShader(pig_roam3, SND_CHANNEL_ANY, 0, false, nullptr);
							}
						}
					}
					
				}
			}
			break;

		case PIGCOP_IDLE_ROAR:
			Event_SetState("state_ApproachingEnemy");
			return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}
