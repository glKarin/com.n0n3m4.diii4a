#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_gameblock.h"

#define GB_RAISETIME				400

#define DISENGAGE_DISTANCETHRESHOLD	8

#define DISTANCE_FROM_PLAYEREYE		10

CLASS_DECLARATION(idStaticEntity, idGameblock)
END_CLASS

//BC this is the little game console thing in the hub level.

idGameblock::idGameblock(void)
{
}

idGameblock::~idGameblock(void)
{
}

void idGameblock::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_CORPSE);
	GetPhysics()->SetClipMask(MASK_SOLID);

	isFrobbable = true;
	fl.takedamage = false; //let's make our lives easier and make cassette tapes invincible

	state = GB_IDLE;

	Event_SetGuiInt("noninteractive", 1);

	startPos = GetPhysics()->GetOrigin();
	startAngle = GetPhysics()->GetAxis().ToAngles();

	BecomeActive(TH_THINK);
}

void idGameblock::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteVec3( startPos ); // idVec3 startPos
	savefile->WriteAngles( startAngle ); // idAngles startAngle

	savefile->WriteVec3( targetPos ); // idVec3 targetPos
	savefile->WriteAngles( targetAngle ); // idAngles targetAngle

	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteFloat( distanceToPlayerEye ); // float distanceToPlayerEye
}

void idGameblock::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state

	savefile->ReadVec3( startPos ); // idVec3 startPos
	savefile->ReadAngles( startAngle ); // idAngles startAngle

	savefile->ReadVec3( targetPos ); // idVec3 targetPos
	savefile->ReadAngles( targetAngle ); // idAngles targetAngle

	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadFloat( distanceToPlayerEye ); // float distanceToPlayerEye
}

void idGameblock::Think(void)
{
	// SW 3rd March 2025
	// This is the event that tells the game gui to update its state.
	// The key here is that the game *will not* update if the game block itself can't think.
	// So the game doesn't update if time is stopped (e.g. if the player is in the pause menu)
	if (this->renderEntity.gui[0])
	{
		this->renderEntity.gui[0]->HandleNamedEvent("updateGame");
	}
	

	if (state == GB_RAISINGUP)
	{
		float lerpAmount = (gameLocal.time - stateTimer) / (float)GB_RAISETIME;
		lerpAmount = idMath::ClampFloat(0, 1, lerpAmount);
		lerpAmount = idMath::CubicEaseOut(lerpAmount);

		idVec3 lerpedPos;
		lerpedPos.Lerp(startPos, targetPos, lerpAmount);			
		SetOrigin(lerpedPos);

		idAngles lerpedAngle;
		lerpedAngle.pitch = idMath::Lerp(startAngle.pitch, targetAngle.pitch, lerpAmount);
		lerpedAngle.yaw = idMath::Lerp(startAngle.yaw, targetAngle.yaw, lerpAmount);
		lerpedAngle.roll = idMath::Lerp(startAngle.roll, targetAngle.roll, lerpAmount);
		SetAxis(lerpedAngle.ToMat3());
		
		if (gameLocal.time >= stateTimer + GB_RAISETIME)
		{
			state = GB_UP;
			gameLocal.GetLocalPlayer()->Event_setPlayerFrozen(0);
			Event_SetGuiInt("noninteractive", 0); //is now fully up. make it interactive.
			StartSound("snd_music", SND_CHANNEL_MUSIC); // Start game music
			distanceToPlayerEye = GetDistanceFromBlock();
		}
	}
	else if (state == GB_UP)
	{
		//Check for conditions to lower/deactivate the block.


		idVec3 forward, up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
		idVec3 frontOfMachinePos = GetPhysics()->GetOrigin() + (forward * (DISTANCE_FROM_PLAYEREYE - 1)) + (up * 4);		

		idVec3 toPlayer = frontOfMachinePos - idVec3(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().x, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().y, frontOfMachinePos.z);
		toPlayer.Normalize();
		idVec3 machineFacing = GetPhysics()->GetAxis().ToAngles().ToForward();
		float facingResult = DotProduct(toPlayer, machineFacing);		

		

		float curDistToPlayer = GetDistanceFromBlock();
		
		if ((curDistToPlayer > distanceToPlayerEye + DISENGAGE_DISTANCETHRESHOLD) || facingResult > 0)
		{
			//Lower the gameblock.
			SetLowerState();
		}
	}
	else if (state == GB_LOWERINGDOWN)
	{
		float lerpAmount = (gameLocal.time - stateTimer) / (float)GB_RAISETIME;
		lerpAmount = idMath::ClampFloat(0, 1, lerpAmount);
		lerpAmount = idMath::CubicEaseOut(lerpAmount);

		idVec3 lerpedPos;
		lerpedPos.Lerp(targetPos, startPos,  lerpAmount);
		SetOrigin(lerpedPos);

		idAngles lerpedAngle;
		lerpedAngle.pitch = idMath::Lerp(targetAngle.pitch, startAngle.pitch,  lerpAmount);
		lerpedAngle.yaw = idMath::Lerp(targetAngle.yaw, startAngle.yaw,  lerpAmount);
		lerpedAngle.roll = idMath::Lerp(targetAngle.roll, startAngle.roll,  lerpAmount);
		SetAxis(lerpedAngle.ToMat3());

		if (gameLocal.time >= stateTimer + GB_RAISETIME)
		{
			state = GB_IDLE;
			isFrobbable = true;
			StopSound(SND_CHANNEL_MUSIC, 0); // Stop game music
		}
	}

	idStaticEntity::Think();
}

void idGameblock::SetLowerState()
{
	if (state == GB_IDLE)
		return;

	state = GB_LOWERINGDOWN;
	stateTimer = gameLocal.time;
	Event_SetGuiInt("noninteractive", 1);

	StartSound("snd_handle", SND_CHANNEL_BODY);
}

idVec3 idGameblock::GetFrontOfBlock()
{
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	return GetPhysics()->GetOrigin() + (forward * 4) + (up * 4);
}

float idGameblock::GetDistanceFromBlock()
{	
	idVec3 startPos = GetFrontOfBlock();	

	return (startPos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin).Length();
}

bool idGameblock::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL)
	{
		return false;
	}

	if (frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	targetAngle = idAngles(0, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);;
	targetAngle.pitch = 0;
	targetAngle.yaw += 180;
	targetAngle.roll = 0;
	targetAngle.Normalize180();

	targetPos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin + targetAngle.ToForward() * -DISTANCE_FROM_PLAYEREYE;
	targetPos.z -= 4;

	stateTimer = gameLocal.time;
	state = GB_RAISINGUP;

	isFrobbable = false;

	gameLocal.GetLocalPlayer()->Event_setPlayerFrozen(2);
	gameLocal.GetLocalPlayer()->GetPhysics()->SetLinearVelocity(vec3_zero);
	gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, GB_RAISETIME);


	//lower any other active game block
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idGameblock::Type))
		{
			if (ent->entityNumber == this->entityNumber)
				continue;

			static_cast<idGameblock*>(ent)->SetLowerState();
		}
	}

	StartSound("snd_handle", SND_CHANNEL_BODY);
	
	return true;
}

