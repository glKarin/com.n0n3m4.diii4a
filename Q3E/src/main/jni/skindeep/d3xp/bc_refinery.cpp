#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
//#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "bc_refinery.h"

const int CHARGETIME = 3000;
const int SPRAYTIME = 5000;
const int PRESPRAYTIME = 200;
const int POSTSPRAYTIME = 500;

const int  SPRAY_TRACE_RADIUS = 14;

CLASS_DECLARATION(idMover, idRefinery)
	EVENT(EV_Activate, idRefinery::Event_Activate)
END_CLASS

idRefinery::idRefinery(void)
{
	bafflerNode.SetOwner(this);
	bafflerNode.AddToEnd(gameLocal.bafflerEntities);
}

idRefinery::~idRefinery(void)
{
	bafflerNode.Remove();
}

void idRefinery::Spawn(void)
{
	int i;
	idVec3 forwardDir, rightDir;
	idDict algaeArgs, lightArgs;

	fl.takedamage = true;
	refineryState = REFINERY_IDLE;
	initialized = false;	

	Event_SetMoveSpeed(spawnArgs.GetInt("speed", "64"));
	Event_SetAccellerationTime(spawnArgs.GetInt("accel_time", "2"));
	Event_SetDecelerationTime(spawnArgs.GetInt("decel_time", "2"));

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);

	for (i = 0; i < 4; i++)
	{
		idDict args;
		idVec3 offset;
		idAngles smokeAng = this->GetPhysics()->GetAxis().ToAngles();
		float downOffset = spawnArgs.GetFloat("smoke_offset_down", "94");
		float sideOffset = spawnArgs.GetFloat("smoke_offset_side", "70");

		offset = GetPhysics()->GetOrigin() + idVec3(0, 0, -downOffset);

		if (i == 0)
		{
			offset += forwardDir * sideOffset;
		}
		else if (i == 1)
		{
			offset += forwardDir * -sideOffset;
			smokeAng.yaw += 180;
		}
		else if (i == 2)
		{
			offset += rightDir * sideOffset;
			smokeAng.yaw += -90;
		}
		else
		{
			offset += rightDir * -sideOffset;
			smokeAng.yaw += 90;
		}

		//tilt smoke downward.
		smokeAng.pitch += 135;

		args.Set("model", "smokepuff02.prt");
		args.SetInt("start_off", 1);
		smokeEmitters[i] = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
		smokeEmitters[i]->GetPhysics()->SetOrigin(offset );
		smokeEmitters[i]->SetAngles(smokeAng);
		//smokeEmitters[i]->PostEventMS(&EV_Activate, 0, this);
		smokeEmitters[i]->Bind(this, true);
	}

	algaeArgs.Set("model", "refinery_spew.prt");
	algaeArgs.SetInt("start_off", 1);
	algaeEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &algaeArgs));
	algaeEmitter->GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, spawnArgs.GetFloat("emitter_offset", "-214")));
	algaeEmitter->SetAngles(idAngles(180,0,0));
	algaeEmitter->Bind(this, true);
	//algaeEmitter->PostEventMS(&EV_Activate, 0, this);

	algaeArgs.Clear();
	algaeArgs.Set("model", "refinery_splash.prt");
	algaeArgs.SetInt("start_off", 1);
	splashEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &algaeArgs));

	
	lightArgs.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0,0, spawnArgs.GetFloat("spotlight_offset", "-224")));
	lightArgs.Set("texture", "lights/refinery_siren");
	lightArgs.SetInt("noshadows", 1);
	lightArgs.SetInt("start_off", 1);
	lightArgs.Set("_color", "1 .5 0 1");
	//spotlight settings.
	lightArgs.Set("light_right", "160 0 0");
	lightArgs.Set("light_target", "0 0 -256");
	lightArgs.Set("light_up", "0 160 0");
	sirenLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
	sirenLight->Bind(this, false);

	skins[0] = declManager->FindSkin("skins/objects/refinery/strip_off");
	skins[1] = declManager->FindSkin("skins/objects/refinery/strip_on");
	
	rotationCounter = 0;
	
	if (spawnArgs.GetBool("start_on") == false)
	{
		refineryState = REFINERY_STATIC;
	}	

	splashTimer = 0;
	BecomeActive(TH_THINK);
}

void idRefinery::Event_Activate(idEntity* activator)
{
	if (refineryState == REFINERY_STATIC)
	{
		refineryState = REFINERY_IDLE;
	}
}

void idRefinery::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( refineryState ); // int refineryState

	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteBool( initialized ); // bool initialized
	savefile->WriteObject( pathEnt ); // idEntityPtr<idEntity> pathEnt
	savefile->WriteInt( rotationCounter ); // int rotationCounter

	SaveFileWriteArray( smokeEmitters, 4, WriteObject ); // idFuncEmitter *smokeEmitters[4]
	SaveFileWriteArray( skins, 2, WriteSkin ); // const idDeclSkin *skins[2]

	savefile->WriteObject( algaeEmitter ); // idFuncEmitter * algaeEmitter
	savefile->WriteObject( splashEmitter ); // idFuncEmitter * splashEmitter

	savefile->WriteObject( sirenLight ); // idLight * sirenLight

	savefile->WriteInt( splashTimer ); // int splashTimer
}

void idRefinery::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( refineryState ); // int refineryState

	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadBool( initialized ); // bool initialized
	savefile->ReadObject( pathEnt ); // idEntityPtr<idEntity> pathEnt
	savefile->ReadInt( rotationCounter ); // int rotationCounter

	SaveFileReadArrayCast( smokeEmitters, ReadObject, idClass*& ); // idFuncEmitter *smokeEmitters[4]
	SaveFileReadArray( skins, ReadSkin ); // const idDeclSkin *skins[2]

	savefile->ReadObject( CastClassPtrRef(algaeEmitter) ); // idFuncEmitter * algaeEmitter
	savefile->ReadObject( CastClassPtrRef(splashEmitter) ); // idFuncEmitter * splashEmitter

	savefile->ReadObject( CastClassPtrRef(sirenLight) ); // idLight * sirenLight

	savefile->ReadInt( splashTimer ); // int splashTimer
}

void idRefinery::Think(void)
{
	idMover::Think();

	if (refineryState == REFINERY_IDLE)
	{
		idVec3 targetPosition;

		if (!initialized)
		{
			//Get the first path point
			initialized = true;

			if (targets.Num() <= 0)
			{
				//No paths here.... should do a special static mode where the refinery just doesn't move.
				refineryState = REFINERY_MOVING;
			}
			else
			{
				pathEnt = targets[0].GetEntity();
			}
		}
		else
		{
			//Find the next point.
			if (pathEnt.GetEntity()->targets.Num() > 0)
			{
				pathEnt = pathEnt.GetEntity()->targets[0].GetEntity();
			}
			else
			{
				//No more paths available... turn into static.
				refineryState = REFINERY_MOVING;
			}
		}		

		//go to next path point.
		targetPosition = pathEnt.GetEntity()->GetPhysics()->GetOrigin();
		Event_MoveToPos(targetPosition);
		idAngles angles(idVec3(0, rotationCounter * 179, 0));
		Event_RotateTo(angles);

		if (rotationCounter <= 1)
		{
			rotationCounter = 2;
		}
		else
		{
			rotationCounter = 1;
		}
		
		refineryState = REFINERY_MOVING;
	}
	else if (refineryState == REFINERY_MOVING)
	{
		if (!Event_IsMoving())
		{
			int i;
			refineryState = REFINERY_CHARGING;

			for (i = 0; i < 4; i++)
			{
				stateTimer = gameLocal.time;
				smokeEmitters[i]->SetActive(true);
			}

			StartSound("snd_whistle", SND_CHANNEL_BODY, 0, false, NULL);
			spawnArgs.SetInt("baffleactive", BAFFLE_CAMOUFLAGED);

			SetSkin(skins[1]);
			sirenLight->On();
		}
	}
	else if (refineryState == REFINERY_CHARGING)
	{
		if (gameLocal.time >= stateTimer + CHARGETIME)
		{
			int i;
			refineryState = REFINERY_SPRAYING;
			stateTimer = gameLocal.time + PRESPRAYTIME;

			for (i = 0; i < 4; i++)
			{
				stateTimer = gameLocal.time;
				smokeEmitters[i]->SetActive(false);
			}

			

			StartSound("snd_goop", SND_CHANNEL_BODY2, 0, false, NULL);

			algaeEmitter->SetActive(true);
			splashEmitter->SetActive(true);
		}
	}
	else if (refineryState == REFINERY_SPRAYING)
	{
		if (gameLocal.time > splashTimer)
		{
			trace_t tr;
			int			surfaceType;

			splashTimer = gameLocal.time + 200;

			//Check where the goop lands.
			gameLocal.clip.TraceBounds(tr, GetPhysics()->GetOrigin() + idVec3(0, 0, -spawnArgs.GetFloat("smoke_offset_down", "94")), GetPhysics()->GetOrigin() + idVec3(0, 0, -1024),
				idBounds(idVec3(-SPRAY_TRACE_RADIUS, -SPRAY_TRACE_RADIUS, -1), idVec3(SPRAY_TRACE_RADIUS, SPRAY_TRACE_RADIUS , 1 )),
				MASK_SOLID | MASK_SHOT_RENDERMODEL, this);

			splashEmitter->SetOrigin(tr.endpos);

			surfaceType = tr.c.material != NULL ? tr.c.material->GetSurfaceType() : SURFTYPE_NONE;
			if (surfaceType != SURFTYPE_LIQUID)
			{
				gameLocal.ProjectDecal(tr.endpos, -tr.c.normal, 8.0f, true, 40, "textures/decals/algaepuddle");
			}
		}

		if (gameLocal.time >= stateTimer + SPRAYTIME)
		{
			refineryState = REFINERY_POSTSPRAY;
			SetSkin(skins[0]);
			sirenLight->Off();
			algaeEmitter->SetActive(false);
			splashEmitter->SetActive(false);

			StopSound(SND_CHANNEL_BODY2, false);
			StartSound("snd_spindown", SND_CHANNEL_BODY, 0, false, NULL);
			spawnArgs.SetInt("baffleactive", 0);

			stateTimer = gameLocal.time + POSTSPRAYTIME;
		}
	}
	else if (refineryState == REFINERY_POSTSPRAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			if (spawnArgs.GetBool("loop"))
			{
				refineryState = REFINERY_IDLE;
			}
			else
			{
				refineryState = REFINERY_STATIC; // wait for activation before moving again
			}
		}
	}

	
}

void idRefinery::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	
}

