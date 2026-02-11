#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "WorldSpawn.h"
#include "bc_trashfish.h"

const int TRASH_EJECTIONDISTANCE = 512; //IMPORTANT: make sure this is synced up with TRASHCUBE_EJECTIONDISTANCE in bc_trashairlock.cpp.

const int SUCKLING_INTERVALTIME = 400;

const float ATTACH_VARIATION = 60;

const float MOVETIME = 1000.0f;
const float MOVETIME_VARIATION = 1500.0f;

const int SEARCHBOX_SIZE = 32;

const int AIRLOCK_HALFLENGTH = 192; //Controls the fish attachment points.



const int PERCH_MAXDISTANCE = 384; //max radius from hive that that fish will perch.
const int PERCH_MINDISTANCE_TO_ENTITIES = 48; //fish are prohibited from being within XX units to certain entities (hive, trashchute exit)

const int OXYGENBUBBLE_SPAWNTIME_MIN = 10000; //min random time to poop oxygenbubble.
const int OXYGENBUBBLE_SPAWNTIME_MAX = 30000; //max random time to poop oxygenbubble.

CLASS_DECLARATION(idAnimated, idTrashfish)

END_CLASS


idTrashfish::idTrashfish(void)
{
}

idTrashfish::~idTrashfish(void)
{
}

void idTrashfish::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	fl.takedamage = true;
	Event_PlayAnim("idle", 1, true);

	stateTimer = 0;
	fishState = TRASHFISHSTATE_IDLE;

	bubbleTimer = 0;
}

void idTrashfish::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( airlockEnt ); // idEntityPtr<idEntity> airlockEnt

	savefile->WriteInt( fishState ); // int fishState
	savefile->WriteInt( fishIndex ); // int fishIndex

	savefile->WriteObject( targetCube ); // idEntityPtr<idEntity> targetCube

	savefile->WriteInt( totalMovetime ); // int totalMovetime
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteVec3( startPosition ); // idVec3 startPosition
	savefile->WriteVec3( endPosition ); // idVec3 endPosition

	savefile->WriteObject( hiveOwner ); // idEntityPtr<idEntity> hiveOwner

	savefile->WriteInt( bubbleTimer ); // int bubbleTimer
}

void idTrashfish::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( airlockEnt ); // idEntityPtr<idEntity> airlockEnt

	savefile->ReadInt( fishState ); // int fishState
	savefile->ReadInt( fishIndex ); // int fishIndex

	savefile->ReadObject( targetCube ); // idEntityPtr<idEntity> targetCube

	savefile->ReadInt( totalMovetime ); // int totalMovetime
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadVec3( startPosition ); // idVec3 startPosition
	savefile->ReadVec3( endPosition ); // idVec3 endPosition

	savefile->ReadObject( hiveOwner ); // idEntityPtr<idEntity> hiveOwner

	savefile->ReadInt( bubbleTimer ); // int bubbleTimer
}

void idTrashfish::Think(void)
{
	idAnimated::Think();	

	if (fishState == TRASHFISHSTATE_IDLE)
	{

		//Idle state. If there's a cube available, then go to the cube. Otherwise, idle to the airlock.
		//if (DoCubeCheck())
		//{
		//	StopSound(SND_CHANNEL_ANY, false);
		//	fishState = TRASHFISHSTATE_HEADING_TO_CUBE;
		//	return;
		//}
	
		//No cube found. Go to a spot on the airlock.
		MoveToPos(GetValidPerchingSpot());
		fishState = TRASHFISHSTATE_HEADING_TO_AIRLOCK;
	}
	else if (fishState == TRASHFISHSTATE_HEADING_TO_AIRLOCK)
	{
		idVec3 newPos;
		float lerp = (gameLocal.time - stateTimer) / (float)totalMovetime;

		if (lerp < 0) { lerp = 0; }
		if (lerp > 1) { lerp = 1; }

		//Move the fish.
		newPos.Lerp(startPosition, endPosition, lerp);
		GetPhysics()->SetOrigin(newPos);

		if (lerp >= 1)
		{
			//Is done with move. Has arrived at the airlock.
			idAngles fishAngle;
			
			//Set its perpendicular to airlock.
			if (airlockEnt.IsValid())
			{
				fishAngle = airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
				fishAngle.roll = gameLocal.random.RandomFloat() * 359;
				GetPhysics()->SetAxis(fishAngle.ToMat3());
			}

			Event_PlayAnim("suckle", 1, true);
			stateTimer = gameLocal.time + 1000;
			fishState = TRASHFISHSTATE_SUCKLING_ON_AIRLOCK;			
			bubbleTimer = gameLocal.time + gameLocal.random.RandomInt(0, OXYGENBUBBLE_SPAWNTIME_MAX);

			StartSound("snd_suck", SND_CHANNEL_VOICE, 0, false, NULL);
		}
	}
	else if (fishState == TRASHFISHSTATE_SUCKLING_ON_AIRLOCK)
	{
		if (!gameLocal.InPlayerConnectedArea(this))
			return;

		if (gameLocal.time > stateTimer)
		{
			//Do a scheduled search for trash to suckle on.
			
			stateTimer = gameLocal.time + 1000 + gameLocal.random.RandomInt(1500);

			if (DoCubeCheck())
			{
				//Found a cube. Great, let's go suckle it. Transition to next state.				
				fishState = TRASHFISHSTATE_HEADING_TO_CUBE;
				StartSound("snd_unsuck", SND_CHANNEL_VOICE, 0, false, NULL);
			}
		}
	
		//if (gameLocal.time > bubbleTimer)
		//{
		//	StartSound("snd_bubblespawn", SND_CHANNEL_ANY);
		//	bubbleTimer = gameLocal.time + gameLocal.random.RandomInt(OXYGENBUBBLE_SPAWNTIME_MIN, OXYGENBUBBLE_SPAWNTIME_MAX);
		//
		//	idVec3 forward;
		//	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		//	idVec3 bubbleSpawnPos = GetPhysics()->GetOrigin() + forward * -24;
		//	idDict args;
		//	args.Clear();
		//	args.SetVector("origin", bubbleSpawnPos);
		//	args.SetVector("movedir", forward);
		//	args.Set("classname", "moveable_oxygenbubble");
		//
		//	idEntity *bubbleEnt;
		//	gameLocal.SpawnEntityDef(args, &bubbleEnt);
		//
		//	if (bubbleEnt)
		//	{
		//		bubbleEnt->GetPhysics()->GetClipModel()->SetOwner(this); //Needs this so the bubble doesnt intersect with the fish.
		//	}			
		//}
	}
	else if (fishState == TRASHFISHSTATE_HEADING_TO_CUBE)
	{
		idBounds cubeProximity;
		idVec3 newPos;

		float lerp = (gameLocal.time - stateTimer) / (float)totalMovetime;

		if (lerp < 0) { lerp = 0; }
		if (lerp > 1) { lerp = 1; }

		//Move the fish.
		newPos.Lerp(startPosition, endPosition, lerp);
		GetPhysics()->SetOrigin(newPos);
	
		//if (targetCube.IsValid())
		//{
		//	cubeProximity = idBounds(targetCube.GetEntity()->GetPhysics()->GetOrigin()).Expand(64);
		//
		//	//gameRenderWorld->DebugBounds(colorYellow, cubeProximity, vec3_origin, 1000);
		//
		//	if (cubeProximity.ContainsPoint(GetPhysics()->GetOrigin()) || lerp >= 1)
		//	{
		//		//Fish is close to the cube. Transition to next state.
		//
		//		//Attach fish to cube. Find a good attachment point.
		//		StartSound("snd_suck", SND_CHANNEL_VOICE2, 0, false, NULL);
		//		AttachFishToCube(targetCube.GetEntity());
		//
		//		stateTimer = gameLocal.time + SUCKLING_INTERVALTIME;
		//		fishState = TRASHFISHSTATE_SUCKLING_ON_CUBE;
		//
		//		StartSound("snd_nibble", SND_CHANNEL_VOICE, 0, false, NULL);
		//
		//
		//		idEntityFx::StartFx("fx/trashfish_nibble", &(GetPhysics()->GetOrigin()), &mat3_identity, NULL, false);
		//	}
		//}
		//else
		//{
		//	//Trashcube is gone. Return to airlock.
		//	StopSound(SND_CHANNEL_ANY, false);
		//	fishState = TRASHFISHSTATE_IDLE;
		//}
	}
	else if (fishState == TRASHFISHSTATE_SUCKLING_ON_CUBE)
	{
		//if (gameLocal.time >= stateTimer)
		//{
		//	stateTimer = gameLocal.time + SUCKLING_INTERVALTIME;
		//
		//	//Check if the cube is dead.
		//	if (!targetCube.IsValid())
		//	{
		//		Unbind();
		//		fishState = TRASHFISHSTATE_IDLE;
		//		StopSound(SND_CHANNEL_ANY, false);
		//		StartSound("snd_unsuck", SND_CHANNEL_VOICE, 0, false, NULL);
		//		return;
		//	}
		//
		//	if (targetCube.GetEntity()->health <= 0)
		//	{
		//		Unbind();
		//		fishState = TRASHFISHSTATE_IDLE;
		//		StopSound(SND_CHANNEL_ANY, false);
		//		StartSound("snd_unsuck", SND_CHANNEL_VOICE, 0, false, NULL);
		//		return;
		//	}
		//
		//	//okay, cube is still there and is alive. Do a little bite.
		//	//targetCube.GetEntity()->health -= 1;
		//	targetCube.GetEntity()->Damage(this, this, vec3_zero, spawnArgs.GetString("damage_eat", "damage_trashfish_gobble"), 1.0f, 0);
		//}
	}
}

//If find a valid cube, then return true and send fish to the cube.
bool idTrashfish::DoCubeCheck()
{
	idEntity *cubeEnt;

	cubeEnt = GetValidCube();
	if (cubeEnt)
	{
		idVec3 randomOffset;
		targetCube = cubeEnt;
		
		//Give it a random offset so the fish don't all move to the same spot.
		randomOffset = idVec3(gameLocal.random.CRandomFloat() * 48, gameLocal.random.CRandomFloat() * 48, gameLocal.random.CRandomFloat() * 48); 
		MoveToPos(cubeEnt->GetPhysics()->GetOrigin() + randomOffset);
		return true;
	}

	return false;
}

void idTrashfish::MoveToPos(idVec3 newPos)
{
	idVec3 newAngle;

	startPosition = GetPhysics()->GetOrigin();
	endPosition = newPos;

	stateTimer = gameLocal.time;
	totalMovetime = MOVETIME + gameLocal.random.RandomFloat() * MOVETIME_VARIATION;

	newAngle = endPosition - startPosition;
	newAngle.Normalize();
	GetPhysics()->SetAxis(newAngle.ToMat3());
}

void idTrashfish::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	trace_t trRay;

	if (!fl.takedamage)
		return;

	fl.takedamage = false;
	PostEventMS(&EV_Remove, 100); //Immediately get rid of it.

	//Particle explosion.


	//Place decal.
	gameLocal.clip.TracePoint(trRay, inflictor->GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin(), MASK_SOLID, NULL);

	if (trRay.fraction < 1 && g_bloodEffects.GetBool())
	{
		gameLocal.ProjectDecal(trRay.endpos, -trRay.c.normal, 8.0f, true, 48, "textures/decals/trashfish_blood");
	}
}

void idTrashfish::SetFishInfo(idEntity * _airlock, int _index)
{
	airlockEnt = _airlock;
	fishIndex = _index;
}

void idTrashfish::SetFishHive(idEntity * _hive)
{
	hiveOwner = _hive;
}

void idTrashfish::AttachFishToCube(idEntity * cube)
{
	//Find a suitable point to attach fish to cube. We do this by
	//checking the center of each side of the cube. Do a distance
	//check and attach to the closest side.

	int i;
	idVec3 forward, right, up;
	idVec3 finalPoint;
	idVec3 newFishAngle;

	cube->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	float closestDistance = 9000;
	int closestIndex = 0;

	idVec3 offsets[] =
	{
		up * 64,		//0 UP.
		up * -64,		//1 DOWN.
		forward * 64,	//2 FORWARD.
		forward * -64,	//3 BACK.
		right * 64,		//4 RIGHT.
		right * -64		//5 LEFT.
	};

	for (i = 0; i < 6; i++)
	{
		float distanceToAttachpoint;
		
		idVec3 attachPoint = cube->GetPhysics()->GetOrigin();
		attachPoint += offsets[i];

		//gameRenderWorld->DebugArrow(colorGreen, attachPoint, GetPhysics()->GetOrigin(), 8, 10000);

		distanceToAttachpoint = (attachPoint - GetPhysics()->GetOrigin()).LengthFast();

		if (distanceToAttachpoint < closestDistance)
		{
			closestDistance = distanceToAttachpoint;
			closestIndex = i;
		}
	}

	//Ok great. We now have a valid attach point. To make it
	//look more organic & natural, we add some random offset.

	finalPoint = cube->GetPhysics()->GetOrigin() + offsets[closestIndex];
	
	if (closestIndex == 0 || closestIndex == 1)
	{
		//Top or bottom.
		finalPoint += forward * (gameLocal.random.CRandomFloat() * ATTACH_VARIATION) + right * (gameLocal.random.CRandomFloat() * ATTACH_VARIATION);
	}
	else if (closestIndex == 2 || closestIndex == 3)
	{
		//Forward or back.
		finalPoint += up * (gameLocal.random.CRandomFloat() * ATTACH_VARIATION) + right * (gameLocal.random.CRandomFloat() * ATTACH_VARIATION);
	}
	else
	{
		//Left or right.
		finalPoint += up * (gameLocal.random.CRandomFloat() * ATTACH_VARIATION) + forward * (gameLocal.random.CRandomFloat() * ATTACH_VARIATION);
	}

	//gameRenderWorld->DebugArrow(colorOrange, GetPhysics()->GetOrigin(), finalPoint, 8, 10000);

	//Set angle and position.
	GetPhysics()->SetOrigin(finalPoint);

	newFishAngle =  cube->GetPhysics()->GetOrigin() - finalPoint;
	newFishAngle.Normalize();
	GetPhysics()->SetAxis(newFishAngle.ToMat3());
	
	//Bind to cube.
	Bind(cube, true);
}

//When done eating, find a space to return to.
idVec3 idTrashfish::GetValidPerchingSpot()
{
	if (!airlockEnt.IsValid())
	{
		for (int i = 0; i < 128; i++)
		{
			//ok, we want to find a random spot to perch NEAR the hive.

			//First find a random point near the hive.
			idVec3 randomDestination;
			randomDestination.x = hiveOwner.GetEntity()->GetPhysics()->GetOrigin().x + gameLocal.random.RandomInt(-PERCH_MAXDISTANCE, PERCH_MAXDISTANCE);
			randomDestination.y = hiveOwner.GetEntity()->GetPhysics()->GetOrigin().y + gameLocal.random.RandomInt(-PERCH_MAXDISTANCE, PERCH_MAXDISTANCE);
			randomDestination.z = hiveOwner.GetEntity()->GetPhysics()->GetOrigin().z + gameLocal.random.RandomInt(-PERCH_MAXDISTANCE, PERCH_MAXDISTANCE);

			//now do a traceline to this random point.
			trace_t tr;
			gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), randomDestination, MASK_SOLID, NULL);
			if (tr.fraction >= 1 || !gameLocal.entities[tr.c.entityNum]->IsType(idWorldspawn::Type)) //make sure point is valid.
				continue;

			//Make sure it's not sky.
			if (tr.c.material->GetSurfaceFlags() >= 256)
				continue; //It's sky. Exit here.


			//make sure it's an appropriate distance to hive.
			float distanceToHive = (hiveOwner.GetEntity()->GetPhysics()->GetOrigin() - tr.endpos).LengthFast();
			if (distanceToHive <= PERCH_MINDISTANCE_TO_ENTITIES && distanceToHive <= PERCH_MAXDISTANCE)
				continue;

			//Make sure it's not close to any trash exit.
			int closestDistance = 9999;
			for (int i = 0; i < gameLocal.trashexitEntities.Num(); i++)
			{
				idVec3 trashExitPos = gameLocal.trashexitEntities[i]->GetPhysics()->GetOrigin();
				float distanceToTrashExit = (trashExitPos - tr.endpos).LengthFast();
				if (distanceToTrashExit < closestDistance)
					closestDistance = distanceToTrashExit;
			}
			if (closestDistance < PERCH_MINDISTANCE_TO_ENTITIES)
				continue;

			return tr.endpos;
		}

		//uh oh.... fail???

		//this is terrible, but we'll just smush the fish onto the hive.
		return hiveOwner.GetEntity()->GetPhysics()->GetOrigin();
	}

	//Find a spot on the airlock to suckle on.
	int randomSide = gameLocal.random.RandomInt(3);
	idVec3 forward, right, up;
	idVec3 spot;

	airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	spot = airlockEnt.GetEntity()->GetPhysics()->GetOrigin() + forward * -AIRLOCK_HALFLENGTH;

	if (randomSide == 0)
	{
		//Find a spot on the TOP.
		spot += up * 170;
		spot += right * gameLocal.random.RandomFloat() * 80;
		return spot;
	}	
	else if (randomSide == 1)
	{
		//Find a spot on the LEFT.
		spot += right * 90;
		spot += up * (gameLocal.random.RandomFloat() * 160);
		return spot;
	}
	else if (randomSide == 2)
	{
		//Find a spot on the RIGHT.
		spot += right * -90;
		spot += up * (gameLocal.random.RandomFloat() * 160);
		return spot;
	}
	//else if (randomSide == 3)
	//{
	//	//Find a spot on the BOTTOM.
	//	spot += up * -10;
	//	spot += right * gameLocal.random.RandomFloat() * 80;
	//	return spot;
	//}
	else
	{
		gameLocal.Error("idTrashfish::GetValidAirlockSpot invalid random number\n");
	}

	return vec3_zero;
}

//Detect whether a delicious trash cube is floating in outer space, waiting to be gobbled up by me...........
idEntity * idTrashfish::GetValidCube()
{
	if (!airlockEnt.IsValid())
		return NULL;

	idEntity	*entityList[MAX_GENTITIES];
	int			listedEntities, i;
	idBounds	searchBounds;
	idVec3		forward, searchPos;

	airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	searchPos = airlockEnt.GetEntity()->GetPhysics()->GetOrigin() + (forward * -TRASH_EJECTIONDISTANCE) + idVec3(0,0,64);
	searchBounds = idBounds(searchPos);
	searchBounds = searchBounds.Expand(32);

	//gameRenderWorld->DebugBounds(colorRed, searchBounds, vec3_origin, 10000);

	listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(searchBounds, entityList, MAX_GENTITIES);

	if (listedEntities > 0)
	{
		for (i = 0; i < listedEntities; i++)
		{
			idEntity *ent = entityList[i];

			if (!ent)
				continue;

			if (ent->spawnArgs.GetInt("trashcube") <= -1 && ent->health > 0)
				return ent;				
		}
	}

	return NULL;
}

