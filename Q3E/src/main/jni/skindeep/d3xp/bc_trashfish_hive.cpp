#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "framework/DeclEntityDef.h"

#include "bc_trashfish.h"
#include "bc_trashfish_hive.h"

const int FISHHIVE_SPAWNTIMER = 1500;
const int FISHHIVE_SPAWNOFFSET = 64; //How far from hive to spawn the fish.

const int FISHHIVE_SPAWNANIMTIME = 1000;

CLASS_DECLARATION(idAnimated, idTrashfishHive)
	//EVENT(EV_PostSpawn, idTrashfishHive::Event_PostSpawn)
END_CLASS

//The trashfish hive spawns trashfish. It tries to keep six of them in the world. If it detects less than six, it'll spawn more fish to refill the fish quota.
idTrashfishHive::idTrashfishHive(void)
{
}

idTrashfishHive::~idTrashfishHive(void)
{
}

void idTrashfishHive::Spawn(void)
{
	fishspawnTimer = 0;
	PostEventMS(&EV_PostSpawn, 0); //Need this for post-spawn to work!
	BecomeActive(TH_THINK);

	hiveState = HIVE_IDLE;

	Event_PlayAnim("idle", 1, true);

	PostEventMS(&EV_PostSpawn, 0);
}


//void idTrashfishHive::Event_PostSpawn(void) //We need to do this post-spawn because not all ents exist when Spawn() is called. So, we need to wait until AFTER spawn has happened, and call this post-spawn function.
//{
//	//Set the airlock it's assigned to.
//	if (targets.Num() <= 0)
//	{
//		gameLocal.Error("trashfish hive %s is not assigned to an airlock.", GetName());
//		return;
//	}
//
//	airlockEnt = targets[0].GetEntity();
//
//	if (!airlockEnt.IsValid())
//	{
//		gameLocal.Error("trashfish hive %s airlock doesn't seem to exist in map.", GetName());
//		return;
//	}
//
//	//Ok, we now have an airlock assigned to this.
//}

void idTrashfishHive::Save(idSaveGame *savefile) const
{
	SaveFileWriteArray( fishes, MAXTRASHFISH, WriteObject ); // idEntityPtr<idEntity> fishes[MAXTRASHFISH]
	savefile->WriteInt( fishspawnTimer ); // int fishspawnTimer
	savefile->WriteInt( idleTimer ); // int idleTimer
	savefile->WriteInt( hiveState ); // int hiveState
}

void idTrashfishHive::Restore(idRestoreGame *savefile)
{
	SaveFileReadArray( fishes, ReadObject ); // idEntityPtr<idEntity> fishes[MAXTRASHFISH]
	savefile->ReadInt( fishspawnTimer ); // int fishspawnTimer
	savefile->ReadInt( idleTimer ); // int idleTimer
	savefile->ReadInt( hiveState ); // int hiveState
}

void idTrashfishHive::Think(void)
{
	idAnimated::Think();

	if (gameLocal.time > fishspawnTimer)
	{
		int i;

		fishspawnTimer = gameLocal.time + FISHHIVE_SPAWNTIMER;

		//Check if all my fish are alive.
		for (i = 0; i < MAXTRASHFISH; i++)
		{
			if (fishes[i].IsValid())
			{
				continue; //Fish is alive. It's doing fine. Skip it.
			}
			else
			{
				//Fish in this slot is NONEXISTENT or DEAD. Spawn a new one to replace it.
				const idDeclEntityDef *fishDef;
				idEntity *fishEnt;

				fishDef = gameLocal.FindEntityDef("monster_trashfish", false);
				gameLocal.SpawnEntityDef(fishDef->dict, &fishEnt, false);

				if (fishEnt)
				{
					idVec3 forward;

					if (!fishEnt->IsType(idTrashfish::Type)) //Ensure we just spawned a trashfish.
					{
						gameLocal.Error("trashfish hive %s attempted to spawn a non-trashfish entity.", GetName());
						return;
					}

					this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

					fishEnt->GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() + forward * FISHHIVE_SPAWNOFFSET); //Where it appears.
					//static_cast<idTrashfish*>(fishEnt)->SetFishInfo(airlockEnt.GetEntity(), i); //assign airlock to the fish.
					static_cast<idTrashfish*>(fishEnt)->SetFishHive(this);
					fishEnt->UpdateVisuals();
					fishes[i] = fishEnt; //Assign it to a slot.

					Event_PlayAnim("spawn", 1, false);

					hiveState = HIVE_SPAWNING;
					idleTimer = gameLocal.time + FISHHIVE_SPAWNANIMTIME;

					return; //Spawned one fish. Exit the loop, wait until next respawn wave.
				}
			}
		}
	}

	if (hiveState == HIVE_SPAWNING)
	{
		if (gameLocal.time > idleTimer)
		{
			hiveState = HIVE_IDLE;
			Event_PlayAnim("idle", 1, true);
		}
	}
}

void idTrashfishHive::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
}

