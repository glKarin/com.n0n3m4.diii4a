#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "framework/DeclEntityDef.h"

#include "bc_trashcuber.h"


//TODO: turn off when not in player pvs.

const int MOVESPEED = 64;
const int SLAMSPEED = 1000;
const int RISESPEED = 48;

const float INTERIORLIGHT_FADETIME = 2; //in seconds.
const float LIGHT_TRANSITIONTIME = .5f; //in seconds.

const int CHARGEUPTIME = 2000;

const int MAKECUBETIME = 2500;

CLASS_DECLARATION(idMover, idTrashcuber)
END_CLASS

idTrashcuber::idTrashcuber(void)
{
}

idTrashcuber::~idTrashcuber(void)
{
}

void idTrashcuber::Spawn(void)
{
	int i;
	idDict lightArgs;
	idVec3 forwardDir, rightDir;

	//Spawn red light inside the machine.
	lightArgs.Clear();
	lightArgs.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0, 0, 96));
	lightArgs.SetInt("noshadows", 1);
	lightArgs.SetInt("start_off", 0);
	lightArgs.Set("_color", ".5 .5 .5 1");
	interiorLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
	interiorLight->Bind(this, false);
	interiorLight->SetRadius(128);

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);

	//Spawn outside lights.
	for (i = 0; i < 4; i++)
	{
		idVec3 lightPos = GetPhysics()->GetOrigin() + idVec3(0, 0, 10);
		lightArgs.Clear();
		
		if (i == 0)
			lightArgs.SetVector("origin", lightPos + forwardDir * 90 );
		else if (i == 1)
			lightArgs.SetVector("origin", lightPos + forwardDir * -90);
		else if (i == 2)
			lightArgs.SetVector("origin", lightPos + rightDir * 90);
		else
			lightArgs.SetVector("origin", lightPos + rightDir * -90);

		lightArgs.SetInt("noshadows", 1);
		lightArgs.Set("texture", "lights/refinery_siren");
		lightArgs.SetInt("start_off", 1);
		exteriorLights[i] = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);

		if (i == 0)
			exteriorLights[i]->SetAxis(idAngles(-90, 0, 0).ToMat3());
		else if (i == 1)
			exteriorLights[i]->SetAxis(idAngles(90, 0, 0).ToMat3());
		else if (i == 2)
			exteriorLights[i]->SetAxis(idAngles(90, 90, 0).ToMat3());
		else
			exteriorLights[i]->SetAxis(idAngles(-90, 90, 0).ToMat3());

		exteriorLights[i]->Bind(this, true);
		exteriorLights[i]->SetRadius(48);
	}

	lightArgs.Clear();
	lightArgs.SetVector("origin", GetPhysics()->GetOrigin());
	lightArgs.Set("texture", "textures/lights/trash_spotlight");
	lightArgs.SetInt("noshadows", 0);
	lightArgs.SetInt("start_off", 0);
	lightArgs.Set("_color", "1 0 0 1");
	lightArgs.Set("light_right", "96 0 0");
	lightArgs.Set("light_target", "0 0 -768");
	lightArgs.Set("light_up", "0 96 0");
	spotlight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
	spotlight->Bind(this, true);
	
	


	//Set the skin to OFF.
	SetSkin(declManager->FindSkin("skins/models/objects/trashcuber/skin"));

	//Turn lightcolor off.
	SetColor(idVec3(.43f, .35f, 0));
	
	cuberState = CUBER_IDLE;
	stateTimer = gameLocal.time + 2000;

	

	if (spawnArgs.GetBool("active", "1"))
	{
		BecomeActive(TH_THINK);
	}
}

void idTrashcuber::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( interiorLight ); // idLight * interiorLight
	SaveFileWriteArray( exteriorLights, 4, WriteObject ); // idLight * exteriorLights[4]
	savefile->WriteObject( spotlight ); // idLight * spotlight

	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( cuberState ); // int cuberState

	savefile->WriteInt( lastZPosition ); // int lastZPosition

	savefile->WriteString( targetAirlock ); // const char * targetAirlock
}

void idTrashcuber::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( CastClassPtrRef(interiorLight) ); // idLight * interiorLight
	SaveFileReadArrayCast( exteriorLights, ReadObject, idClass*& ); // idLight * exteriorLights[4]
	savefile->ReadObject( CastClassPtrRef(spotlight) ); // idLight * spotlight

	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( cuberState ); // int cuberState

	savefile->ReadInt( lastZPosition ); // int lastZPosition

	savefile->ReadString( targetAirlock ); // const char * targetAirlock
}

//We constantly resize the spotlight so that it remains the same consistent size, regardless of how high or low the trashcuber is from the ground.
void idTrashcuber::UpdateSpotlightSize()
{
	trace_t downTr;

	gameLocal.clip.TracePoint(downTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0, 0, -1024), CONTENTS_SOLID, this);

	if (downTr.fraction < 1)
	{
		if (downTr.c.entityNum <= MAX_GENTITIES - 2 && downTr.c.entityNum >= 0)
		{
			if (gameLocal.entities[downTr.c.entityNum]->IsType(idWorldspawn::Type)) //Only update spotlight size if it hits world brushes.
			{
				float lightDistance = downTr.fraction * -1024;
				spotlight->SetLightTarget(idVec3(0, 0, lightDistance));
			}
		}
	}	
}

idVec3 idTrashcuber::FindValidPosition(void)
{
	//Try to find a valid position for the cuber to move to. 
	int i;
	for (i = 0; i < 8; i++) //Attempt a few times.
	{
		//Pick a random spot and check it.
		int j;
		int randomIndex = gameLocal.random.RandomInt(targets.Num() - 1);
		for (j = randomIndex; j < targets.Num(); j++)
		{
			idEntity *targetEnt = targets[j].GetEntity();
			trace_t downTr;

			if (!targetEnt)
				continue;

			//Do a trace downward.
			gameLocal.clip.TracePoint(downTr, targetEnt->GetPhysics()->GetOrigin(), targetEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, -2048), MASK_SOLID, this);

			if (downTr.fraction >= 1.0f)
				continue; //This ... shouldn't happen? But if it does, then just ignore the spot.

			if (downTr.c.entityNum <= MAX_GENTITIES - 2 && downTr.c.entityNum >= 0)
			{
				if (gameLocal.entities[downTr.c.entityNum]->IsType(idMover::Type))
				{
					const idKeyValue *keyval = gameLocal.entities[downTr.c.entityNum]->spawnArgs.FindKey("model");

					if (!idStr::Cmp(keyval->GetValue(), "models/objects/trashcube/trashcube.ase"))
					{
						continue;
					}
				}
			}
			
			if (targetEnt->targets.Num() > 0)
			{
				targetAirlock = targetEnt->targets[0].GetEntity()->GetName();
			}
			else
			{
				gameLocal.Error("trash cuber %s attempted to use node %s, but node had no airlock assigned to it.", GetName(), targetEnt->GetName());
			}

			return targetEnt->GetPhysics()->GetOrigin();
		}
	}

	//Fail. No suitable position found.
	return vec3_zero;
}

void idTrashcuber::Think(void)
{
	idMover::Think();	

	if (cuberState == CUBER_IDLE)
	{
		if (gameLocal.time > stateTimer)
		{
			idVec3 movePos;
			int i;

			cuberState = CUBER_MOVING;
			stateTimer = 0;

			//Move to new location.
			Event_SetMoveSpeed(MOVESPEED);

			movePos = FindValidPosition();

			if (movePos == vec3_zero)
			{
				//Failed to find a position. Stay idle. Wait for a bit of time.
				cuberState = CUBER_IDLE;
				stateTimer = gameLocal.time + 5000;

				//Turn off lights.
				SetColor(idVec3(.43f, .35f, 0));
				SetSkin(declManager->FindSkin("skins/models/objects/trashcuber/skin"));
				interiorLight->Fade(idVec4(.5f, .5f, .5f, 1), INTERIORLIGHT_FADETIME);
				for (i = 0; i < 4; i++)
				{
					exteriorLights[i]->FadeOut(LIGHT_TRANSITIONTIME);
				}

				return;
			}
			
			//Do move order.
			Event_MoveToPos(movePos);

			//Change colors.
			SetColor(idVec3(1, 1, 0));
			SetSkin(declManager->FindSkin("skins/models/objects/trashcuber/skin_on"));
			interiorLight->Fade(idVec4(1, 1, 0, 1), LIGHT_TRANSITIONTIME);

			for (i = 0; i < 4; i++)
			{
				exteriorLights[i]->SetColor(idVec3(1, 1, 0));
				exteriorLights[i]->On();				
			}

			spotlight->SetColor(idVec3(1, 1, 0));
		}
	}
	else if (cuberState == CUBER_MOVING)
	{
		UpdateSpotlightSize();

		if (!Event_IsMoving())
		{
			//Stops moving.
			int i;

			cuberState = CUBER_CHARGEDELAY;
			stateTimer = gameLocal.time + 1500;

			//Change colors.
			SetColor(idVec3(1, 0, 0));
			interiorLight->Fade(idVec4(1, 0, 0, 1), LIGHT_TRANSITIONTIME);
			for (i = 0; i < 4; i++)
			{
				exteriorLights[i]->Fade(idVec4(1, 0, 0, 1), LIGHT_TRANSITIONTIME);
			}
			spotlight->Fade(idVec4(1, 0, 0, 1), LIGHT_TRANSITIONTIME);
			spotlight->SetShader("textures/lights/trash_spotlight_flash");
			

			//Make loud sound.

			//Spawn crud particles.
			idEntityFx::StartFx("fx/trash_debris", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

			StartSound("snd_movestop", SND_CHANNEL_ITEM, 0, false, NULL);
			StartSound("snd_alarm", SND_CHANNEL_BODY3, 0, false, NULL);
		}
	}
	else if (cuberState == CUBER_CHARGEDELAY)
	{
		//A little delay before entering chargeup state.

		if (gameLocal.time > stateTimer)
		{
			idVec3 forwardDir, rightDir;
			int i;
			idMat3 downDir;

			cuberState = CUBER_CHARGINGUP;
			stateTimer = gameLocal.time + CHARGEUPTIME;

			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);

			downDir = idAngles(180, 0, 0).ToMat3();

			for (i = 0; i < 4; i++)
			{
				idVec3 particlePos = GetPhysics()->GetOrigin();

				if (i == 0)
					particlePos += idVec3(72, 72, 0);
				else if (i == 1)
					particlePos += idVec3(72, -72, 0);
				else if (i == 2)
					particlePos += idVec3(-72, -72, 0);
				else
					particlePos += idVec3(-72, 72, 0);

				idEntityFx::StartFx("fx/smokepuff02_quick", &particlePos, &downDir, NULL, false);
			}
		}
	}
	else if (cuberState == CUBER_CHARGINGUP)
	{
		//Charge up....

		if (gameLocal.time > stateTimer)
		{
			//Start the slamdown.
			trace_t downTr;
			idVec3 slamPosition;
			
			//Calculate if we have a place to slam to.
			//Determine where we slam down to.
			gameLocal.clip.TracePoint(downTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0,0,-1024), MASK_SOLID, this);

			if (downTr.fraction >= 1)
			{
				//Trace didn't hit anything.
				cuberState = CUBER_IDLE;
				stateTimer = gameLocal.time + 1000;
				return;
			}

			lastZPosition = GetPhysics()->GetOrigin().z;

			cuberState = CUBER_SLAMMINGDOWN;

			slamPosition = downTr.endpos;			
			Event_SetMoveSpeed(SLAMSPEED);
			Event_MoveToPos(slamPosition);

			StartSound("snd_slammove", SND_CHANNEL_BODY3, 0, false, NULL);
		}
	}
	else if (cuberState == CUBER_SLAMMINGDOWN)
	{
		//Is slamming downward.

		if (!Event_IsMoving())
		{
			int i;
			idVec3 forwardDir, rightDir;
			idAngles particleAng;
			//Has completed the slamdown move.

			//Make slam sound.

			//Do particle fx.
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);
			for (i = 0; i < 4; i++)
			{
				idVec3 particleAngle;
				idVec3 smokePos = GetPhysics()->GetOrigin() + idVec3(0, 0, 10);

				if (i == 0)
				{
					smokePos += forwardDir * 90;
				}
				else if (i == 1)
				{
					smokePos += forwardDir * -90;
				}
				else if (i == 2)
				{
					smokePos += rightDir * 90;
				}
				else
				{
					smokePos += rightDir * -90;
				}

				particleAngle = smokePos - GetPhysics()->GetOrigin();
				particleAngle.Normalize();
				particleAng = particleAngle.ToAngles();
				particleAng.pitch += 80;

				idEntityFx::StartFx("fx/trash_windblow2", smokePos, particleAng.ToMat3());
			}
			

			//Spawn trash cube.
			SpawnTrashcube();

			StartSound("snd_slamhit", SND_CHANNEL_BODY3, 0, false, NULL);
			StartSound("snd_makingcube", SND_CHANNEL_BODY2, 0, false, NULL);

			cuberState = CUBER_MAKINGCUBE;
			stateTimer = gameLocal.time + MAKECUBETIME;
		}
	}
	else if (cuberState == CUBER_MAKINGCUBE)
	{
		//Making the cube.

		if (gameLocal.time > stateTimer)
		{
			//Start rising upward.
			int i;
			idVec3 raisedPosition = GetPhysics()->GetOrigin();
			raisedPosition.z = lastZPosition;

			Event_SetMoveSpeed(RISESPEED);
			Event_MoveToPos(raisedPosition);


			//Change colors.
			SetColor(idVec3(1, 1, 0));
			interiorLight->Fade(idVec4(1, 1, 0, 1), LIGHT_TRANSITIONTIME);
			for (i = 0; i < 4; i++)
			{
				exteriorLights[i]->Fade(idVec4(1, 1, 0, 1), LIGHT_TRANSITIONTIME);
			}
			spotlight->SetColor(idVec3(1, 1, 0));
			spotlight->SetShader("textures/lights/trash_spotlight");

			idEntityFx::StartFx("fx/trash_debris2", GetPhysics()->GetOrigin() + idVec3(0,0,256), mat3_identity);

			cuberState = CUBER_RISINGUP;
		}
	}
	else if (cuberState == CUBER_RISINGUP)
	{
		//Is rising back up.

		if (!Event_IsMoving())
		{
			//Has finished rising up.
			
			cuberState = CUBER_IDLE;
			stateTimer = gameLocal.time + 1000;
		}
	}
}

void idTrashcuber::SpawnTrashcube()
{
	const idDeclEntityDef *cubeDef;
	idEntity *cubeEnt;

	cubeDef = gameLocal.FindEntityDef("env_trashcube", false);
	gameLocal.SpawnEntityDef(cubeDef->dict, &cubeEnt, false);

	if (cubeEnt)
	{
		cubeEnt->SetOrigin(this->GetPhysics()->GetOrigin() + idVec3(0, 0, 64));
		cubeEnt->spawnArgs.Set("target", targetAirlock);
		cubeEnt->FindTargets(); //This is called to commit the 'target' field.
	}

	//Todo: Randomize the angle
}