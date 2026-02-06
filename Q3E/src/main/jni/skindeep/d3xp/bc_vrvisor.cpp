#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "ai/AI.h"

#include "bc_vrvisor.h"


CLASS_DECLARATION(idMoveableItem, idVRVisor)
END_CLASS

#define VISOR_MOVETIME 600
#define VISOR_MOVEPAUSETIME 150
#define VISOR_MOVINGTOEYES_TIME 300


#define VISOR_RETURNTIME 500

#define VISOR_FORWARDDIST 8

idVRVisor::idVRVisor(void)
{
	state = VRV_IDLE;
	stateTimer = 0;

	visorStartPosition = vec3_zero;
	visorStartAngle = idAngles(0, 0, 0);

	visorFinalPosition = vec3_zero;
	visorFinalAngle = idAngles(0, 0, 0);

	playerStartPosition = vec3_zero;
	playerStartAngle = 0;

	arrowProp = NULL;
}

idVRVisor::~idVRVisor(void)
{
}

void idVRVisor::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteVec3( visorStartPosition ); // idVec3 visorStartPosition
	savefile->WriteAngles( visorStartAngle ); // idAngles visorStartAngle

	savefile->WriteVec3( visorFinalPosition ); // idVec3 visorFinalPosition
	savefile->WriteAngles( visorFinalAngle ); // idAngles visorFinalAngle

	savefile->WriteVec3( playerStartPosition ); // idVec3 playerStartPosition
	savefile->WriteFloat( playerStartAngle ); // float playerStartAngle

	savefile->WriteObject( arrowProp ); // idEntity* arrowProp
}

void idVRVisor::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadVec3( visorStartPosition ); // idVec3 visorStartPosition
	savefile->ReadAngles( visorStartAngle ); // idAngles visorStartAngle

	savefile->ReadVec3( visorFinalPosition ); // idVec3 visorFinalPosition
	savefile->ReadAngles( visorFinalAngle ); // idAngles visorFinalAngle

	savefile->ReadVec3( playerStartPosition ); // idVec3 playerStartPosition
	savefile->ReadFloat( playerStartAngle ); // float playerStartAngle

	savefile->ReadObject( arrowProp ); // idEntity* arrowProp
}

void idVRVisor::Spawn(void)
{
	isFrobbable = true;	

	BecomeActive(TH_THINK);
	fl.takedamage = false;


	if (spawnArgs.GetBool("showarrow", "1"))
	{
		idDict args;
		args.Set("classname", "func_static");
		args.Set("model", spawnArgs.GetString("model_arrow"));
		args.SetBool("spin", true);
		args.Set("bind", GetName());
		args.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0,0,3));
		args.SetBool("solid", false);
		gameLocal.SpawnEntityDef(args, &arrowProp);
	}


	PostEventMS(&EV_PostSpawn, 0);
}

void idVRVisor::Event_PostSpawn(void)
{
	//This does not work for some reason??????
	//if (targets.Num() <= 0)
	//{
	//	int zz = targets.Num();
	//	gameLocal.Error("visor '%s' has no targets.", GetName(), zz);
	//}
}



void idVRVisor::Think(void)
{
	if (state == VRV_IDLE)
	{
		idMoveableItem::Think();
	}
	else if (state == VRV_MOVINGTOPLAYER)
	{
		Present();


		//Get visor final position.
		idAngles playerViewAngle = gameLocal.GetLocalPlayer()->viewAngles;
		playerViewAngle.pitch = 0;
		playerViewAngle.roll = 0;
		idVec3 playerEyePos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		visorFinalPosition = playerEyePos + playerViewAngle.ToForward() * VISOR_FORWARDDIST;
		visorFinalPosition.z += .1f;

		//Get visor final angle.
		visorFinalAngle = idAngles(0, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);;
		visorFinalAngle.pitch = 0;
		visorFinalAngle.yaw += 180;
		visorFinalAngle.roll = 0;
		visorFinalAngle.Normalize180();


		float lerp = (gameLocal.time - stateTimer) / (float)VISOR_MOVETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = idMath::CubicEaseOut(lerp);



		//Lerp visor position.
		idVec3 visorLerpedPosition;
		visorLerpedPosition.Lerp(visorStartPosition, visorFinalPosition, lerp);
		SetOrigin(visorLerpedPosition);

		//Lerp visor angle.
		idAngles visorLerpedAngle;
		visorLerpedAngle.pitch = idMath::Lerp(visorStartAngle.pitch, visorFinalAngle.pitch, lerp);
		visorLerpedAngle.yaw = idMath::Lerp(visorStartAngle.yaw, visorFinalAngle.yaw, lerp);
		visorLerpedAngle.roll = idMath::Lerp(visorStartAngle.roll, visorFinalAngle.roll, lerp);
		SetAxis(visorLerpedAngle.ToMat3());		
		
		if (gameLocal.time > stateTimer + VISOR_MOVETIME)
		{
			state = VRV_MOVEPAUSE;
			stateTimer = gameLocal.time;
		}
	}
	else if (state == VRV_MOVEPAUSE)
	{
		if (gameLocal.time > stateTimer + VISOR_MOVEPAUSETIME)
		{
			state = VRV_MOVINGTOEYES;
			stateTimer = gameLocal.time;
			gameLocal.GetLocalPlayer()->Event_SetFOVLerp(-80, VISOR_MOVINGTOEYES_TIME);

			StartSound("snd_enter", SND_CHANNEL_ANY);
		}
	}
	else if (state == VRV_MOVINGTOEYES)
	{
		if (gameLocal.time > stateTimer + VISOR_MOVINGTOEYES_TIME)
		{
			state = VRV_ATTACHEDTOPLAYER;
			gameLocal.GetLocalPlayer()->Event_setPlayerFrozen(0);

			if (targets.Num() > 0)
			{
				gameLocal.GetLocalPlayer()->FlashScreenCustom(idVec4(0, .6f, .8f, 1), 600);
				gameLocal.GetLocalPlayer()->Event_SetFOVLerp(40, 0);
				gameLocal.GetLocalPlayer()->Event_SetFOVLerp(0, 500);
				gameLocal.GetLocalPlayer()->Event_TeleportToEnt(targets[0].GetEntity());

				//Call the script to start the sequence.
				idStr startScript = spawnArgs.GetString("call_start");
				if (startScript.Length() > 0)
				{
					if (!gameLocal.RunMapScriptArgs(startScript.c_str(), this, this))
					{
						gameLocal.Warning("visor '%s' failed to run script '%s'", GetName(), startScript.c_str());
					}
				}
			}
			else
			{
				gameLocal.Error("visor '%s' has no targets.", GetName());
			}
		}
	}
	else if (state == VRV_RETURNINGTOSTARTPOSITION)
	{
		Present();

		float lerp = (gameLocal.time - stateTimer) / (float)VISOR_RETURNTIME;
		lerp = idMath::ClampFloat(0, 1, lerp);

		//Lerp visor position.
		idVec3 visorLerpedPosition;
		visorLerpedPosition.Lerp(visorFinalPosition, visorStartPosition,  lerp);
		SetOrigin(visorLerpedPosition);

		//Lerp visor angle.
		idAngles visorLerpedAngle;
		visorLerpedAngle.pitch = idMath::Lerp( visorFinalAngle.pitch, visorStartAngle.pitch, lerp);
		visorLerpedAngle.yaw = idMath::Lerp( visorFinalAngle.yaw, visorStartAngle.yaw, lerp);
		visorLerpedAngle.roll = idMath::Lerp(visorFinalAngle.roll, visorStartAngle.roll, lerp);
		SetAxis(visorLerpedAngle.ToMat3());

		if (gameLocal.time > stateTimer + VISOR_RETURNTIME)
		{
			state = VRV_IDLE;
			//isFrobbable = true; //bc 11-12-2024 just make the vr visor be a one-use only item.
		}
	}

}



bool idVRVisor::DoFrob(int index, idEntity * frobber)
{
	//idMoveableItem::DoFrob(index, frobber);

	if (frobber == NULL)
		return false;

	if (frobber != gameLocal.GetLocalPlayer())
		return false;

	if (state == VRV_IDLE)
	{
		if (arrowProp != nullptr)
		{
			arrowProp->Hide();
		}

		gameLocal.GetLocalPlayer()->Event_setPlayerFrozen(1);
		gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, VISOR_MOVETIME+ VISOR_MOVINGTOEYES_TIME + VISOR_MOVEPAUSETIME);
		gameLocal.GetLocalPlayer()->SetViewYawLerp(gameLocal.GetLocalPlayer()->viewAngles.yaw, VISOR_MOVETIME + VISOR_MOVINGTOEYES_TIME + VISOR_MOVEPAUSETIME);

		state = VRV_MOVINGTOPLAYER;
		stateTimer = gameLocal.time;
		visorStartPosition = GetPhysics()->GetOrigin();
		visorStartAngle = GetPhysics()->GetAxis().ToAngles();

		isFrobbable = false;

		//BC 6-11-2025: fix issue where player can go out of bounds by exiting a clambered/crouch space while interacting with vr visor (sd-654)
		playerStartPosition = GetSafeStartPosition();		

		playerStartAngle = gameLocal.GetLocalPlayer()->viewAngles.yaw;

		//Get visor final position.
		idAngles playerViewAngle = gameLocal.GetLocalPlayer()->viewAngles;
		playerViewAngle.pitch = 0;
		playerViewAngle.roll = 0;
		idVec3 playerEyePos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		visorFinalPosition = playerEyePos + playerViewAngle.ToForward() * VISOR_FORWARDDIST;
		visorFinalPosition.z += .1f;

		//Get visor final angle.
		visorFinalAngle = idAngles(0, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);;
		visorFinalAngle.pitch = 0;
		visorFinalAngle.yaw += 180;
		visorFinalAngle.roll = 0;
		visorFinalAngle.Normalize180();

		StartSound("snd_grab", SND_CHANNEL_ANY);
	}


	return true;
}

//BC 6-11-2025: fix issue where player can go out of bounds by exiting a clambered/crouch space while interacting with vr visor (sd-654)
idVec3 idVRVisor::GetSafeStartPosition()
{
	//See if player is already standing on solid ground.
	idVec3 candidateStartPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

	trace_t tr;
	gameLocal.clip.TracePoint(tr, candidateStartPosition + idVec3(0, 0, 1), candidateStartPosition + idVec3(0, 0, -16), MASK_SOLID, NULL);

	if (tr.fraction < 1)
	{
		return candidateStartPosition; //player is on ground.
	}

	//Player is NOT on ground. Find the ground.
	gameLocal.clip.TracePoint(tr, candidateStartPosition + idVec3(0, 0, 1), candidateStartPosition + idVec3(0, 0, -128), MASK_SOLID, NULL);
	candidateStartPosition = tr.endpos + idVec3(0, 0, .1f);

	//Get player standing bounding box
	idBounds playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	playerbounds[1].z = pm_normalheight.GetFloat(); //Get standing height.
	
	#define CANDIDATEOFFSETCOUNT 9
	idVec3 candidateOffsets[] =
	{
		idVec3(0, 0, 0),

		idVec3(0, -16, 0),
		idVec3(0, -32, 0),

		idVec3(-16, 0, 0),
		idVec3(-32, 0, 0),

		idVec3(0, 16, 0),
		idVec3(0, 32, 0),

		idVec3(16, 0, 0),
		idVec3(32, 0, 0),
	};

	//do bounds check.	
	for (int i = 0; i < CANDIDATEOFFSETCOUNT; i++)
	{
		trace_t standingTr;
		idVec3 adjustedPos = candidateStartPosition + candidateOffsets[i];
		gameLocal.clip.TraceBounds(standingTr, adjustedPos, adjustedPos, playerbounds, MASK_SOLID, NULL);

		if (standingTr.fraction >= 1)
		{
			return adjustedPos;
		}
	}

	//Couldn't find a clear spot, so just return the original position on the ground.
	return candidateStartPosition;
}

//If this visor is active, then deactivate it and return player to the hub.
bool idVRVisor::SetExitVisor()
{
	if (state != VRV_ATTACHEDTOPLAYER)
		return false;

	state = VRV_RETURNINGTOSTARTPOSITION;
	stateTimer = gameLocal.time;

	gameLocal.GetLocalPlayer()->FlashScreenCustom(idVec4(0, 0, 0, 1), 200);	
	gameLocal.GetLocalPlayer()->Teleport(playerStartPosition, idAngles(0, playerStartAngle,0), NULL);

	return true;
}