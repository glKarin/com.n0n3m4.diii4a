//#include "sys/platform.h"
//#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
//#include "Entity.h"
//#include "Light.h"
//#include "Player.h"
#include "Fx.h"

#include "bc_bossmonster.h"


const int TELEPORT_WARNINGTIME = 5000; //NOTE: make sure the FX file has this same Duration value.


CLASS_DECLARATION(idAI, idBossMonster)

END_CLASS


void idBossMonster::Spawn(void)
{
	bossState = BOSSSTATE_DORMANT;
	bossStateTimer = 0;
}


void idBossMonster::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( bossState ); //  int bossState
	savefile->WriteInt( bossStateTimer ); //  int bossStateTimer
	savefile->WriteVec3( bossSpawnPos ); //  idVec3 bossSpawnPos
}
void idBossMonster::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( bossState ); //  int bossState
	savefile->ReadInt( bossStateTimer ); //  int bossStateTimer
	savefile->ReadVec3( bossSpawnPos ); //  idVec3 bossSpawnPos
}

void idBossMonster::Think(void)
{
	idAI::Think();

	
	if (bossState == BOSSSTATE_TELPORTCHARGE)
	{
		//Currently in the teleport chargeup state.

		if (gameLocal.time > bossStateTimer)
		{
			//Teleport the boss here.
			
			idVec3 dirToPlayer = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - bossSpawnPos;
			idAngles spawnAngle = dirToPlayer.ToAngles(); //Face toward player.

			this->Teleport(bossSpawnPos, spawnAngle, NULL);
			this->SetState("State_Combat"); //Force boss to enter combat state.

			idEntityFx::StartFx("fx/tele_flash", &bossSpawnPos, &mat3_identity, NULL, false);

			bossState = BOSSSTATE_ACTIVE;			
		}
	}
}


void idBossMonster::StartBossTeleportSequence()
{
	spawnSpot_t		spot;
	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnspots;

	//Gather up all the boss spawnpoints. Put them in a big list.
	spot.ent = gameLocal.FindEntityUsingDef(NULL, "info_bossspawnpoint");
	if (!spot.ent)
	{
		//Uh oh.... no respawn point found.
		gameLocal.Error("Failed to find any info_bossspawnpoint in map.");
		return;
	}

	while (spot.ent)
	{
		//We only care about the spawnpoints that have the "combat" flag. If there is no "combat" flag, then it's an idle spawnpoint for when the
		//boss returns to the bridge.
		if (spot.ent->spawnArgs.GetBool("combat", "0"))
		{
			//Calculate distance and store it.
			float dist = (spot.ent->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();

			if (!gameLocal.InPlayerConnectedArea(spot.ent))
				dist *= 4; //if not in PVS, then we significantly deprioritize it by increasing its distance value.

			//todo: clearance check

			spot.dist = dist;
			spawnspots.Append(spot);
		}

		spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_bossspawnpoint");
	}

	qsort((void *)spawnspots.Ptr(), spawnspots.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Farthest);

	//Boss spawnpoint debug. Show spawnpoint rank and distance.
	//for (int i = 0; i < spawnspots.Num(); i++)
	//{
	//	gameRenderWorld->DrawText(idStr::Format("%d - %d", i, spawnspots[i].dist), spawnspots[i].ent->GetPhysics()->GetOrigin(), 0.5f, idVec4(1, 1, 1, 1), mat3_default, 1, 10000);
	//}

	//Find a random point within the closest spawnpoints (second half of this list).
	int randomIdx = (spawnspots.Num() / 2) + gameLocal.random.RandomInt((spawnspots.Num() / 2));
	spot = spawnspots[randomIdx];
	bossSpawnPos = spot.ent->GetPhysics()->GetOrigin();
	
	//spawn fx.
	idEntityFx::StartFx("fx/tele_warning", &spot.ent->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	bossState = BOSSSTATE_TELPORTCHARGE;
	bossStateTimer = gameLocal.time + TELEPORT_WARNINGTIME;
}

void idBossMonster::StartBossExitSequence()
{
	if (bossState != BOSSSTATE_ACTIVE)
		return;

	spawnSpot_t		spot;
	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnspots;

	bossState = BOSSSTATE_DORMANT;

	spot.ent = gameLocal.FindEntityUsingDef(NULL, "info_bossspawnpoint");

	if (!spot.ent)
	{
		//Uh oh.... no respawn point found.
		gameLocal.Error("Failed to find any info_bossspawnpoint in map.");
		return;
	}

	while (spot.ent)
	{
		//We only care about the spawnpoints that do NOT have the "combat" flag.
		if (!spot.ent->spawnArgs.GetBool("combat", "0"))
		{
			//todo: clearance check		
			spawnspots.Append(spot);
		}

		spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_bossspawnpoint");
	}

	int randomIdx = gameLocal.random.RandomInt(spawnspots.Num());
	spot = spawnspots[randomIdx];

	//Teleport Boss outta here.......
	idEntityFx::StartFx("fx/tele_flash", &this->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

	this->Teleport(spot.ent->GetPhysics()->GetOrigin(), idAngles(0,0,0), NULL);	
	this->SetState("State_Idle"); //Return to idle.
}