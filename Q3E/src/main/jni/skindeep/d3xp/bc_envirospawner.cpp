#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_envirospawner.h"

CLASS_DECLARATION(idEntity, idEnviroSpawner)
END_CLASS

const int TRACELENGTH = 4096;
const int SPAWNBOX_MARGIN = 80;
const int SPAWNBOX_DEPTH = 1;

const int DISTANCE_CHECKAMOUNT = 192; //Make sure consecutive spawns don't happen near each other. Ensure this minimum distance away from one another.

const int SPAWNMARGIN = 192; //Buffer space between the brush and things spawned. Increase this to make things spawn farther away from brush.

const int SPAWN_ATTEMPTS = 16; //we attempt a few times, as we want to prevent things spawning too close to one another consecutively.

const int SPAWN_BOUNDCHECK = 130;

void idEnviroSpawner::Spawn(void)
{
	spawnBox = idBounds(vec3_zero);

	//This assumes the envirospawner is facing SOUTH. By default, maps *SHOULD* have the spaceship facing north (90), and the envirospawner facing south (270).

	//depth. It's paper-thin, just one unit. Since it's fictionally spawning from the skybox, we just make it spawn close to the skybox edge.
	trace_t tr;
	gameLocal.clip.TracePoint(tr, this->GetPhysics()->GetOrigin(), this->GetPhysics()->GetOrigin() + idVec3(0, 512, 0), MASK_SOLID, NULL);
	spawnBox[0].y = tr.endpos.y - SPAWNMARGIN;
	spawnBox[1].y = tr.endpos.y - SPAWNMARGIN + 1;

	//width
	spawnBox[0].x = this->GetPhysics()->GetAbsBounds()[0].x;
	spawnBox[1].x = this->GetPhysics()->GetAbsBounds()[1].x;

	//height.
	spawnBox[0].z = this->GetPhysics()->GetAbsBounds()[0].z;
	spawnBox[1].z = this->GetPhysics()->GetAbsBounds()[1].z;

	timer = 0;
	lastSpawnPos = vec3_zero;


	GetPhysics()->SetContents(0); //so nothing collides with it.

	spawnRate = spawnArgs.GetInt("spawnRateMs", "2000");
	despawnThreshold = spawnArgs.GetInt("despawnThreshold", "-3000");

	// Default asteroid
	idAsteroidParms parms;
	parms.asteroidArgsName = spawnArgs.GetString("def_asteroid");
	parms.asteroidSpawnArgs = gameLocal.FindEntityDefDict(parms.asteroidArgsName, false);
	float size = parms.asteroidSpawnArgs->GetFloat("bbox_size", "128");
	parms.asteroidBox = idBounds(idVec3(-size / 2, -size / 2, -size / 2), idVec3(size / 2, size / 2, size / 2));
	parms.chanceOfSpawning = 1;
	asteroidEntries.Append(parms);

	// optional additional asteroids
	for (int i = 1; i <= 9; i++)
	{
		idStr defName;
		if (spawnArgs.GetString(va("def_asteroid%d", i), "", defName))
		{
			parms.asteroidArgsName = defName;
			parms.asteroidSpawnArgs = gameLocal.FindEntityDefDict(defName, false);
			float size = parms.asteroidSpawnArgs->GetFloat("bbox_size", "128");
			parms.asteroidBox = idBounds(idVec3(-size / 2, -size / 2, -size / 2), idVec3(size / 2, size / 2, size / 2));
			parms.chanceOfSpawning = spawnArgs.GetFloat(va("def_asteroid%d_spawnchance", i), "0");
			asteroidEntries.Append(parms);
		}
	}

	if (spawnArgs.GetBool("start_on"))
	{
		BecomeActive(TH_THINK);
		state = ESP_ON;
	}
	else
	{
		BecomeInactive(TH_THINK);
		state = ESP_OFF;
	}

	PostEventMS(&EV_PostSpawn, 0);
}

void idEnviroSpawner::Event_PostSpawn(void)
{
	// Set up occluder boxes 
	// (this needs to be delayed to postspawn or else the target list will be empty)
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* target = targets[i].GetEntity();
		if (idStr("func_envirospawner_occluder").Icmp(target->spawnArgs.GetString("classname")) == 0)
		{
			idBounds bounds = target->GetPhysics()->GetAbsBounds();

			// Flatten it down to the same 'plane' as the spawnbox
			bounds[0].y = spawnBox[0].y;
			bounds[1].y = spawnBox[1].y;

			occluderBoxes.Append(bounds);
			target->PostEventMS(&EV_Remove, 0);
		}
	}
}

idEnviroSpawner::idEnviroSpawner(void)
{
}

idEnviroSpawner::~idEnviroSpawner(void)
{
	occluderBoxes.Clear();
	asteroidEntries.Clear();
}

void idEnviroSpawner::Save(idSaveGame *savefile) const
{
	savefile->WriteBounds( spawnBox ); //  idBounds spawnBox
	SaveFileWriteArray( occluderBoxes, occluderBoxes.Num(), WriteBounds ); //  idList<idBounds> occluderBoxes

	savefile->WriteInt( asteroidEntries.Num() ); //  idList<idAsteroidParms> asteroidEntries
	for (int idx = 0; idx < asteroidEntries.Num(); idx++)
	{
		savefile->WriteString( asteroidEntries[idx].asteroidArgsName ); // const  idDict*	 asteroidSpawnArgs
		// const  idDict*	 asteroidSpawnArgs // regen
		savefile->WriteBounds( asteroidEntries[idx].asteroidBox ); //  idBounds asteroidBox
		savefile->WriteFloat( asteroidEntries[idx].chanceOfSpawning ); //  float chanceOfSpawning
	}

	savefile->WriteInt( timer ); //  int timer
	savefile->WriteInt( spawnRate ); //  int spawnRate
	savefile->WriteInt( despawnThreshold ); //  int despawnThreshold

	savefile->WriteVec3( lastSpawnPos ); //  idVec3 lastSpawnPos

	savefile->WriteBool( state ); //  bool state
}

void idEnviroSpawner::Restore(idRestoreGame *savefile)
{
	savefile->ReadBounds( spawnBox ); //  idBounds spawnBox
	SaveFileReadList( occluderBoxes, ReadBounds ); //  idList<idBounds> occluderBoxes

	int num;
	savefile->ReadInt( num ); //  idList<idAsteroidParms> asteroidEntries
	asteroidEntries.SetNum(num);
	for (int idx = 0; idx < num; idx++)
	{
		savefile->ReadString( asteroidEntries[idx].asteroidArgsName ); // const  idDict*	 asteroidSpawnArgs
		asteroidEntries[idx].asteroidSpawnArgs = gameLocal.FindEntityDefDict(asteroidEntries[idx].asteroidArgsName, false); // const  idDict*	 asteroidSpawnArgs
		savefile->ReadBounds( asteroidEntries[idx].asteroidBox ); //  idBounds asteroidBox
		savefile->ReadFloat( asteroidEntries[idx].chanceOfSpawning ); //  float chanceOfSpawning
	}

	savefile->ReadInt( timer ); //  int timer
	savefile->ReadInt( spawnRate ); //  int spawnRate
	savefile->ReadInt( despawnThreshold ); //  int despawnThreshold

	savefile->ReadVec3( lastSpawnPos ); //  idVec3 lastSpawnPos

	savefile->ReadBool( state ); //  bool state
}


void idEnviroSpawner::Think(void)
{
	if (state == ESP_OFF)
		return;

	if (gameLocal.time > timer)
	{
		SpawnElement();		
		timer = gameLocal.time + spawnRate;
	}

	//gameRenderWorld->DebugBounds(colorGreen, spawnBox, vec3_zero, 100);
}

void idEnviroSpawner::SpawnElement()
{
	idVec3 randomSpawnPos = vec3_zero;
	randomSpawnPos.y = spawnBox[0].y;

	// Set defaults (these may be randomly overriden by different asteroid entries)
	idBounds asteroidBox = asteroidEntries[0].asteroidBox;
	const idDict* asteroidSpawnArgs = asteroidEntries[0].asteroidSpawnArgs;

	for (int i = 0; i < SPAWN_ATTEMPTS; i++)
	{
		//find a spot that on a "donut shape" around the ship.

		//if (gameLocal.random.RandomFloat() > .5f)
		//{
		//	//spawn rock either on LEFT or RIGHT side of ship.
		//
		//	//int totalLength = spawnBox[1].x - spawnBox[0].x;
		//	if (gameLocal.random.RandomFloat() > .5f)
		//	{
		//		//left side of map.			
		//		randomSpawnPos.x = spawnBox[0].x;
		//	}
		//	else
		//	{
		//		//right side of map.
		//		randomSpawnPos.x = spawnBox[1].x;
		//	}
		//
		//	randomSpawnPos.z = spawnBox[0].z + gameLocal.random.RandomInt(spawnBox[1].z - spawnBox[0].z);
		//}
		//else
		//{
		//	//spawn rock either ABOVE or BELOW ship.
		//
		//	//int totalHeight = spawnBox[1].z - spawnBox[0].z;
		//	if (gameLocal.random.RandomFloat() > .5f)
		//	{
		//		//below ship.
		//		randomSpawnPos.z = spawnBox[0].z;
		//	}
		//	else
		//	{
		//		//above ship.
		//		randomSpawnPos.z = spawnBox[1].z;
		//	}
		//
		//	randomSpawnPos.x = spawnBox[0].x + gameLocal.random.RandomInt(spawnBox[1].x - spawnBox[0].x);
		//}
		
		//pure random.
		randomSpawnPos.x = spawnBox[0].x + gameLocal.random.RandomInt(spawnBox[1].x - spawnBox[0].x);
		randomSpawnPos.y = spawnBox[0].y;
		randomSpawnPos.z = spawnBox[0].z + gameLocal.random.RandomInt(spawnBox[1].z - spawnBox[0].z);
			


		float distanceCheck = (randomSpawnPos - lastSpawnPos).LengthFast();
		if (distanceCheck < DISTANCE_CHECKAMOUNT) //make sure consecutive spawns don't happen right on top of each other. 
			continue;

		// Check it's not inside any of our occluder boxes
		bool insideOccluders = false;
		for (int i = 0; i < occluderBoxes.Num(); i++)
		{
			if (occluderBoxes[i].ContainsPoint(randomSpawnPos))
			{
				insideOccluders = true;
				break;
			}
		}
		if (insideOccluders)
		{
			randomSpawnPos = vec3_zero; // SW 4th June 2025: Prevent spawnpos inside occluder from being used if it's our final attempt
			continue;
		}
			
		
		// Iterate backwards through our list of asteroid entries and see which type we're spawning here
		for (int i = asteroidEntries.Num() - 1; i >= 0; i--)
		{
			// See if we pass our chance of spawning (or if we're down to the default option)
			if (i == 0 || gameLocal.random.RandomFloat() < asteroidEntries[i].chanceOfSpawning)
			{
				asteroidBox = asteroidEntries[i].asteroidBox;
				asteroidSpawnArgs = asteroidEntries[i].asteroidSpawnArgs;
				break;
			}
		}

		//Do a tracebound check.
		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, randomSpawnPos, randomSpawnPos, asteroidBox.Expand(8), MASK_SOLID, NULL);
		if (boundTr.fraction < 1)
			continue;
	}

	if (randomSpawnPos == vec3_zero)
		return;

	lastSpawnPos = randomSpawnPos;

	idEntity *newEnt;
	idDict args;
	args.Set("classname", asteroidSpawnArgs->GetString("classname"));
	args.SetVector("origin", randomSpawnPos);
	args.SetInt("despawnThreshold", despawnThreshold);
	gameLocal.SpawnEntityDef(args, &newEnt);
}