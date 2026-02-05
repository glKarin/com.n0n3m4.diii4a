#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_trashairlock.h"

const int TRASHAIRLOCK_THINKINTERVAL = 500;
const int TRASHAIRLOCK_CHARGEDELAY = 5000;
const int TRASHAIRLOCK_CHARGETIME = 4000;
const int TRASHAIRLOCK_EJECTTIME = 5000;
const float TRASHAIRLOCK_DOORMOVETIME = .3f;
const int TRASHAIRLOCK_DOOR_RETRYTIME = 1500;

const float TRASHCUBE_EJECTTIME = 1.5f;
const float TRASHCUBE_ROTATESPEED = 24;
const int TRASHCUBE_EJECTIONDISTANCE = 512;

const int DOOR_OFFSET = 176;


const idEventDef EV_airlock_prime("Airlock_prime", "e");

CLASS_DECLARATION(idStaticEntity, idTrashAirlock)

	EVENT(EV_airlock_prime, idTrashAirlock::Event_airlock_prime)

END_CLASS

idTrashAirlock::idTrashAirlock(void)
{
	
}

idTrashAirlock::~idTrashAirlock(void)
{
	thinkTimer = 0;
}

void idTrashAirlock::Spawn(void)
{
	int i;
	idVec3 forward, right, up;
	idDict lightArgs;
	int airlockYaw;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	airlockYaw = round(this->GetPhysics()->GetAxis().ToAngles().yaw);

	//Spawn inner doors.
	for (i = 0; i < 3; i++)
	{
		idDict args;

		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * DOOR_OFFSET));
		args.SetInt("no_touch", 1);
		args.SetInt("angle", airlockYaw);

		if (i <= 0)
		{
			args.Set("model", "models/objects/doors/door_c_top.ase");
			args.SetInt("movedir", -1);
			args.SetInt("lip", 8);
		}
		else if (i == 1)
		{
			args.Set("model", "models/objects/doors/door_c_left.ase");
			args.SetInt("movedir", round(right.ToAngles().yaw));
			args.SetInt("lip", 0);
		}
		else
		{
			args.Set("model", "models/objects/doors/door_c_right.ase");
			args.SetInt("movedir", round(right.ToAngles().yaw + 180));
			args.SetInt("lip", 0);
		}

		args.SetFloat("time", TRASHAIRLOCK_DOORMOVETIME);
		args.Set("owner", this->GetName());
		args.Set("team", idStr::Format("%s_innerdoor",this->GetName()));

		if (i <= 0)
		{
			args.Set("snd_opened", "d3_closed");
			args.Set("snd_closed", "d3_closed");
		}

		args.SetInt("wait", -1);
		innerDoor[i] = (idDoor *)gameLocal.SpawnEntityType(idDoor::Type, &args);
	}

	//Spawn outer doors.
	for (i = 0; i < 3; i++)
	{
		idDict args;

		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * -DOOR_OFFSET));
		args.SetInt("no_touch", 1);
		args.SetInt("angle", airlockYaw);
		args.Set("skin", "skins/objects/doors/door_c_outerairlock");

		if (i <= 0)
		{
			args.Set("model", "models/objects/doors/door_c_top.ase");
			args.SetInt("movedir", -1);
		}
		else if (i == 1)
		{
			args.Set("model", "models/objects/doors/door_c_left.ase");
			args.SetInt("movedir", round(right.ToAngles().yaw));
			
		}
		else
		{
			args.Set("model", "models/objects/doors/door_c_right.ase");
			args.SetInt("movedir", round(right.ToAngles().yaw + 180));
		}

		args.SetInt("lip", 0);
		args.SetFloat("time", TRASHAIRLOCK_DOORMOVETIME);
		args.Set("owner", this->GetName());
		args.Set("team", idStr::Format("%s_outerdoor", this->GetName()));

		if (i <= 0)
		{
			args.Set("snd_opened", "d3_closed");
			args.Set("snd_closed", "d3_closed");
		}

		args.SetInt("wait", -1);
		outerDoor[i] = (idDoor *)gameLocal.SpawnEntityType(idDoor::Type, &args);
	}

	//Light.
	lightArgs.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0, 0, 32));
	lightArgs.Set("texture", "lights/defaultPointLight");
	lightArgs.SetInt("noshadows", 1);
	lightArgs.Set("_color", "1 1 1 1");
	lightArgs.SetFloat("light", 200);
	sirenLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
	sirenLight->Bind(this, false);

	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	airlockState = AIRLOCKSTATE_IDLE;
	
	trashcubeEnt = NULL;
	cubeIsSpinning = false;


	if (spawnArgs.GetBool("vacuumseparator", "1"))
	{
		idDict args;
		
		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * DOOR_OFFSET) + (up * 64));
		vacuumSeparators[0] = (idVacuumSeparatorEntity *)gameLocal.SpawnEntityType(idVacuumSeparatorEntity::Type, &args); //inner door vacuumseparator.

		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * -DOOR_OFFSET) + (up * 64));
		vacuumSeparators[1] = (idVacuumSeparatorEntity *)gameLocal.SpawnEntityType(idVacuumSeparatorEntity::Type, &args); //outer door vacuumseparator.
	}


	BecomeActive(TH_THINK);
}

void idTrashAirlock::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( airlockState ); // int airlockState
	SaveFileWriteArray( innerDoor, 3, WriteObject ); // idDoor * innerDoor[3]
	SaveFileWriteArray( outerDoor, 3, WriteObject ); // idDoor * outerDoor[3]

	savefile->WriteInt( thinkTimer ); // int thinkTimer

	savefile->WriteObject( trashcubeEnt ); // idEntityPtr<idEntity> trashcubeEnt

	savefile->WriteObject( sirenLight ); // idLight * sirenLight

	savefile->WriteBool( cubeIsSpinning ); // bool cubeIsSpinning

	SaveFileWriteArray( vacuumSeparators, 2, WriteObject ); // idVacuumSeparatorEntity * vacuumSeparators[2]
}

void idTrashAirlock::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( airlockState ); // int airlockState
	SaveFileReadArrayCast( innerDoor, ReadObject, idClass*& ); // idDoor * innerDoor[3]
	SaveFileReadArrayCast( outerDoor, ReadObject, idClass*& ); // idDoor * outerDoor[3]

	savefile->ReadInt( thinkTimer ); // int thinkTimer

	savefile->ReadObject( trashcubeEnt ); // idEntityPtr<idEntity> trashcubeEnt

	savefile->ReadObject( CastClassPtrRef(sirenLight) ); // idLight * sirenLight

	savefile->ReadBool( cubeIsSpinning ); // bool cubeIsSpinning

	SaveFileReadArrayCast( vacuumSeparators, ReadObject, idClass*&  ); // idVacuumSeparatorEntity * vacuumSeparators[2]
}

void idTrashAirlock::Think(void)
{
	//innerDoor[0]->Open();

	if (airlockState == AIRLOCKSTATE_PRIMED)
	{
		if (gameLocal.time > thinkTimer)
		{
			float distanceToCube;

			if (!trashcubeEnt.GetEntity())
			{
				gameLocal.Warning("trash airlock failed to find its assigned trashcubeEnt.");
				airlockState = AIRLOCKSTATE_IDLE;
				return;
			}

			thinkTimer = gameLocal.time + 1500;

			//Check distance to the trash cube.
			distanceToCube = (trashcubeEnt.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).LengthFast();

			if (distanceToCube <= 70)
			{
				//Cube is in me.
				airlockState = AIRLOCKSTATE_WAITFORINNERDOORCLOSE;
				thinkTimer = 0;
			}
		}
	}
	else if (airlockState == AIRLOCKSTATE_WAITFORINNERDOORCLOSE)
	{
		//Attempt to close inner door. Repeatedly attempt until successful.

		if (gameLocal.time > thinkTimer)
		{
			thinkTimer = gameLocal.time + TRASHAIRLOCK_DOOR_RETRYTIME; //A little delay between attempts.
			innerDoor[0]->Close();

			if (!innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen() && !innerDoor[2]->IsOpen())
			{
				//idVec3 forward;

				StartSound("snd_cycle", SND_CHANNEL_BODY, 0, false, NULL);
				thinkTimer = gameLocal.time + TRASHAIRLOCK_CHARGEDELAY;
				airlockState = AIRLOCKSTATE_CHARGEDELAY;

				sirenLight->Fade(idVec4(1, .4f, 0, 1), 1);

				//this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
				//idEntityFx::StartFx("fx/smoke_ring13", &(GetPhysics()->GetOrigin() + (forward * DOOR_OFFSET) + idVec3(0, 0, 64)), &mat3_identity, NULL, false);
			}
		}
	}
	else if (airlockState == AIRLOCKSTATE_CHARGEDELAY)
	{
		if (gameLocal.time > thinkTimer)
		{
			airlockState = AIRLOCKSTATE_CHARGING;
			thinkTimer = gameLocal.time + TRASHAIRLOCK_CHARGETIME;

			sirenLight->Fade(idVec4(1, 0, 0, 1), .5f);
			sirenLight->SetShader("lights/siren");
			StartSound("snd_alarm", SND_CHANNEL_BODY2, 0, false, NULL);

			SetSkin(declManager->FindSkin("skins/objects/trashairlock/blink"));
		}
	}
	else if (airlockState == AIRLOCKSTATE_CHARGING)
	{
		if (gameLocal.time > thinkTimer )
		{
			idVec3 forward;
			idAngles particleAngle;

			thinkTimer = gameLocal.time + TRASHAIRLOCK_EJECTTIME;
			airlockState = AIRLOCKSTATE_EJECTING;

			//Open the outer doors.
			outerDoor[0]->Open();

			//Particle effect of vacuum particles.
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			particleAngle = GetPhysics()->GetAxis().ToAngles();
			particleAngle.pitch -= 90;
			idEntityFx::StartFx("fx/vacuum_suck2", GetPhysics()->GetOrigin() + (forward * -100) + idVec3(0,0,64), particleAngle.ToMat3());

			//Eject the trash cube into outer space.
			if (trashcubeEnt.GetEntity() && trashcubeEnt.GetEntity()->IsType(idMover::Type))
			{
				idVec3 forward;
				idVec3 trashejectDestination;

				this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
				trashejectDestination = (GetPhysics()->GetOrigin() + idVec3(0, 0, 64)) + (forward * -TRASHCUBE_EJECTIONDISTANCE);

				//gameRenderWorld->DebugArrow(colorYellow, cubeDestination + idVec3(0,0,128), cubeDestination, 8, 5000);

				static_cast<idMover*>(trashcubeEnt.GetEntity())->Event_SetMoveTime(TRASHCUBE_EJECTTIME);
				static_cast<idMover*>(trashcubeEnt.GetEntity())->Event_SetDecelerationTime(TRASHCUBE_EJECTTIME / 2.0f);
				static_cast<idMover*>(trashcubeEnt.GetEntity())->Event_MoveToPos(trashejectDestination);				

				cubeIsSpinning = false;
			}


		}
	}
	else if (airlockState == AIRLOCKSTATE_EJECTING)
	{
		if (trashcubeEnt.IsValid() && !cubeIsSpinning)
		{
			if (!static_cast<idMover*>(trashcubeEnt.GetEntity())->Event_IsMoving())
			{
				static_cast<idMover*>(trashcubeEnt.GetEntity())->Event_SetDecelerationTime(0);
				idAngles angles(gameLocal.random.CRandomFloat() * TRASHCUBE_ROTATESPEED, gameLocal.random.CRandomFloat() * TRASHCUBE_ROTATESPEED, 0);
				static_cast<idMover*>(trashcubeEnt.GetEntity())->Event_Rotate(angles);

				cubeIsSpinning = true;
			}
		}

		if (gameLocal.time > thinkTimer)
		{
			//Close the outer door. Wait for a bit.
			airlockState = AIRLOCKSTATE_WAITFOROUTERDOORCLOSE;
			thinkTimer = 0;
		}
	}
	else if (airlockState == AIRLOCKSTATE_WAITFOROUTERDOORCLOSE)
	{
		//Repeatedly attempt to close outer door. Proceed when outer door successfully closes.
		if (gameLocal.time > thinkTimer)
		{
			thinkTimer = gameLocal.time + TRASHAIRLOCK_DOOR_RETRYTIME; //A little delay between attempts.
			outerDoor[0]->Close();

			if (!outerDoor[0]->IsOpen() && !outerDoor[1]->IsOpen() && !outerDoor[2]->IsOpen())
			{
				StopSound(SND_CHANNEL_BODY2, false);
				SetSkin(declManager->FindSkin("skins/objects/trashairlock/default"));

				airlockState = AIRLOCKSTATE_POSTEJECTION;
				thinkTimer = gameLocal.time + 1500;
			}
		}
	}
	else if (airlockState == AIRLOCKSTATE_POSTEJECTION)
	{
		if (gameLocal.time > thinkTimer)
		{
			//Open the inner door. All done.
			airlockState = AIRLOCKSTATE_IDLE;

			innerDoor[0]->Open();
			sirenLight->SetShader("lights/defaultPointLight");
			sirenLight->Fade(idVec4(1, 1, 1, 1), 3);

			StartSound("snd_done", SND_CHANNEL_BODY, 0, false, NULL);
		}
	}
	

	idStaticEntity::Think();
}

void idTrashAirlock::Event_airlock_prime(idEntity * ent)
{
	//Activate the trash laser.
	//if (trashcubeEnt.GetEntity())
	//{
	//	trashcubeEnt.GetEntity()->PostEventMS(&EV_Remove, 0);
	//}

	if (!innerDoor[0]->IsOpen())
	{
		innerDoor[0]->Open();
	}

	thinkTimer = 0;
	airlockState = AIRLOCKSTATE_PRIMED;

	trashcubeEnt = ent;
}

bool idTrashAirlock::GetReadyStatus(void)
{
	return (airlockState == AIRLOCKSTATE_IDLE);
}