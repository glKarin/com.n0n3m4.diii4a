#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "framework/Session_local.h"

#include "WorldSpawn.h"
#include "Mover.h"
#include "bc_meta.h"
#include "bc_cryointerior.h"
#include "bc_doorbarricade.h"
#include "bc_stresstester.h"

#define GLOBALMAPRESTARTTIME_MINUTES 10
#define GLOBALUNLOCKFUSEBOX_MINUTES 5

#define STUCK_COUNTERTHRESHOLD 6

#define SAVELOAD_DURATION_MIN  "3000"
#define SAVELOAD_DURATION_MAX  "30000"

CLASS_DECLARATION(idEntity, idStressTester)
END_CLASS


idCVar stress_savegame("stress_savegame", "1", CVAR_INTEGER | CVAR_SYSTEM | CVAR_CHEAT, "use savegame during stress test. 1 = random save & load most recent savegame. 2 = random load. 3 = end mission (if hub level). 4 = end mission (all maps).");
idCVar stress_continueafterrestart("stress_continueafterrestart", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_CHEAT, "reactivate stress tester when map restarts");

idCVar stress_savetimer_min("stress_savetimer_min", SAVELOAD_DURATION_MIN, CVAR_INTEGER | CVAR_SYSTEM | CVAR_CHEAT, "time between save load");
idCVar stress_savetimer_max("stress_savetimer_max", SAVELOAD_DURATION_MAX, CVAR_INTEGER | CVAR_SYSTEM | CVAR_CHEAT, "time between save load");

idCVar stress_jockey("stress_jockey", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_CHEAT, "do jockey behavior");
idCVar stress_wander("stress_wander", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_CHEAT, "do wander behavior");

//StressTester
//Make the player randomly wander around the map.
//Auto restarts the map every xx minutes.

//Some bugs only happen when multiple gameplay systems interact with each other. The intention of StressTester is
//to allow the game to be run overnight to catch any of these hard-to-repro gameplay bugs.

void idStressTester::Spawn(void)
{
	GetPhysics()->SetContents(0);
	fl.takedamage = false;
	timer = gameLocal.time + 1000;
	state = CRYOPOD_DELAY;
	pushledgeCount = 0;
	doorTimer = 0;
	jockeyTimer = 0;
	jockeySlamTimer = 0;
	
	

	BecomeActive(TH_THINK);
	common->Printf("\n\n*********** STRESSTEST SPAWNED ***********\n\n");

	globalRestartTimer = gameLocal.time + (GLOBALMAPRESTARTTIME_MINUTES * (1000 * 60));
	globalUnlockFuseboxTimer = gameLocal.time + (GLOBALUNLOCKFUSEBOX_MINUTES * (1000 * 60));
	hasUnlockedGlobalFusebox = false;

	stuckTimer = 0;
	GetUpTimer = 0;
	stuckLastPosition = vec3_zero;
	stuckCounter = 10000;


	saveloadTimer = stress_savetimer_max.GetInteger();
}

void idStressTester::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( timer ); // int timer

	savefile->WriteInt( pushledgeCount ); // int pushledgeCount

	savefile->WriteInt( doorTimer ); // int doorTimer

	savefile->WriteInt( jockeyTimer ); // int jockeyTimer
	savefile->WriteInt( jockeySlamTimer ); // int jockeySlamTimer
	savefile->WriteObject( meleeTarget ); // idEntityPtr<idEntity> meleeTarget

	savefile->WriteInt( globalRestartTimer ); // int globalRestartTimer

	savefile->WriteInt( globalUnlockFuseboxTimer ); // int globalUnlockFuseboxTimer
	savefile->WriteBool( hasUnlockedGlobalFusebox ); // bool hasUnlockedGlobalFusebox

	savefile->WriteInt( GetUpTimer ); // int GetUpTimer

	savefile->WriteInt( stuckTimer ); // int stuckTimer
	savefile->WriteVec3( stuckLastPosition ); // idVec3 stuckLastPosition
	savefile->WriteInt( stuckCounter ); // int stuckCounter

	savefile->WriteInt(saveloadTimer); // int saveloadTimer
}

void idStressTester::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( timer ); // int timer

	savefile->ReadInt( pushledgeCount ); // int pushledgeCount

	savefile->ReadInt( doorTimer ); // int doorTimer

	savefile->ReadInt( jockeyTimer ); // int jockeyTimer
	savefile->ReadInt( jockeySlamTimer ); // int jockeySlamTimer
	savefile->ReadObject( meleeTarget ); // idEntityPtr<idEntity> meleeTarget

	savefile->ReadInt( globalRestartTimer ); // int globalRestartTimer

	savefile->ReadInt( globalUnlockFuseboxTimer ); // int globalUnlockFuseboxTimer
	savefile->ReadBool( hasUnlockedGlobalFusebox ); // bool hasUnlockedGlobalFusebox

	savefile->ReadInt( GetUpTimer ); // int GetUpTimer

	savefile->ReadInt( stuckTimer ); // int stuckTimer
	savefile->ReadVec3( stuckLastPosition ); // idVec3 stuckLastPosition
	savefile->ReadInt( stuckCounter ); // int stuckCounter

	savefile->ReadInt(saveloadTimer); // int saveloadTimer
}

void idStressTester::Think(void)
{
	if (state == CRYOPOD_DELAY)
	{
		if (gameLocal.time > timer)
		{
			//Frob the cryo pod exit.
			state = POSTCRYO_DELAY;
			ExitCryopod();
			timer = gameLocal.time + 1000;
		}
	}
	else if (state == POSTCRYO_DELAY)
	{
		if (gameLocal.time > timer)
		{
			//The player might be on a vent or something.
			//Try to push them off. Push player around in all directions.
			state = PUSHLEDGE;
			pushledgeCount = 0;
			timer = 0;
			common->Printf("Stresstest: jumping.\n");

			gameLocal.GetLocalPlayer()->godmode = true;
		}
	}
	else if (state == PUSHLEDGE)
	{
		//This nudges the player around. This is used at game start because the player sometimes
		//starts on a ledge/catwalk.

		if (gameLocal.time > timer)
		{
			idVec3 pushDir;
			if (pushledgeCount == 0)
				pushDir = idVec3(1, 0, .5f);
			else if (pushledgeCount == 1)
				pushDir = idVec3(-1, 0, .5f);
			else if (pushledgeCount == 2)
				pushDir = idVec3(0, 1, .5f);
			else if (pushledgeCount == 3)
				pushDir = idVec3(0, -1, .5f);

			#define PUSHFORCE 384
			gameLocal.GetLocalPlayer()->GetPhysics()->SetLinearVelocity(pushDir * PUSHFORCE);
			pushledgeCount++;
			timer = gameLocal.time + 900;
			if (pushledgeCount >= 4)
			{
				state = WANDER;

				if (stress_wander.GetBool())
				{
					cvarSystem->SetCVarBool("aas_randomPullPlayer", true);
				}
			}
		}
	}
	else if (state == WANDER)
	{
		//Try to frob any door in front of me.
		if (gameLocal.time > doorTimer)
		{
			doorTimer = gameLocal.time + 2000;
			FrobDoorAttempt();				
		}
		
		//Try to jockey enemies that are nearby.
		if (gameLocal.time > jockeyTimer && stress_jockey.GetBool())
		{
			jockeyTimer = gameLocal.time + 30000;
			if (!gameLocal.GetLocalPlayer()->IsJockeying())
			{
				AttemptJockey();
			}
		}

		//Try to jockey-slam if I am jockeying.
		if (gameLocal.GetLocalPlayer()->IsJockeying())
		{
			if (gameLocal.time > jockeySlamTimer)
			{
				jockeySlamTimer = gameLocal.time + 3000;
				AttemptJockeySlam();
			}
		}

		//See if we need to get up.
		if (gameLocal.time > GetUpTimer)
		{
			GetUpTimer = gameLocal.time + 5000;
			AttemptGetUp();
		}

		UpdateStuckLogic();


		saveloadTimer -= gameLocal.msec; // using subtraction of frame time, since saving/loading can take up variable amounts of gameLocal.time
		if (saveloadTimer <= 0 && stress_savegame.GetInteger() == STS_RANDOMSAVELOAD)
		{
			//BC behavior 1 = do random saving and loading
			saveloadTimer = gameLocal.random.RandomInt(stress_savetimer_min.GetInteger(), stress_savetimer_max.GetInteger());

			//just make the saveload be a random choice.
			bool isSaving = gameLocal.random.RandomInt(100) <= 33;
			// save once first (until sg is finalized, due to outdated save info causing issues, though this might not be an issue with autosave)
			static bool savedOnce = false;
			isSaving = isSaving || !savedOnce;

			if (isSaving)
			{
				//do save.
				savedOnce = true;
				common->Printf("Stresstest: attempting to savegamesession.\n");
				cmdSystem->BufferCommandText(CMD_EXEC_APPEND, va("savegamesession", name.c_str()));
			}
			else
			{
				//do load.
				common->Printf("Stresstest: attempting to LoadLatestSave()\n");
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "loadgamesession" );
				//LoadLatestSave();
			}
		}
		else if (saveloadTimer <= 0 && stress_savegame.GetInteger() == STS_RANDOMLOAD)
		{
			//BC behavior 2 = ONLY do randomly loading game
			LoadRandomSave();			
			return;
		}
		else if (saveloadTimer <= 0 && stress_savegame.GetInteger() == STS_LOADHUBS)
		{
			//BC behavior 3 = end the mission (if hub level), otherwise load
			saveloadTimer = gameLocal.random.RandomInt(stress_savetimer_min.GetInteger(), stress_savetimer_max.GetInteger());

			idStr currentmapname = gameLocal.GetMapNameStripped();
			if (currentmapname.Find("vig_hub", false) >= 0)
			{
				//is hub level.

				//set some random persistent args.
				for (int i = 2; i < 18; i++)
				{
					if (gameLocal.random.RandomInt(2) >= 1)
					{
						gameLocal.GetLocalPlayer()->PickupTape(i);
					}
				}
				
				//end mission.
				cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "debugendmission");
				return;
			}
			else
			{
				LoadRandomSave();
				return;
			}
		}
		else if (saveloadTimer <= 0 && stress_savegame.GetInteger() == STS_LOADALLMAPS)
		{
			//BC behavior 4 = end the mission. Go to next mission. The end result of this is doing an autosave for every level.
			saveloadTimer = gameLocal.random.RandomInt(stress_savetimer_min.GetInteger(), stress_savetimer_max.GetInteger());

			if (gameLocal.GetLocalPlayer()->GetLevelProgressionIndex() >= 22)
			{
				//Game has ended. Return to tutorial to restart the loop.
				gameLocal.GetLocalPlayer()->DebugSetLevelProgressionIndex(0); //BC 4-21-2025: manually reset the campaign progress.
				cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "map vig_tutorial");
				return;
			}

			cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "impulse 25"); //This ends the mission. We use this instead of debugEndmission because impulse25 skips the post-game spectator mode.
			return;
		}


		// unlock one barricade per frame to prevent overflowing event capacity
		for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			if (ent->IsType(idDoorBarricade::Type) && ((idDoorBarricade*)ent)->owningDoor.IsValid() )
			{
				static_cast<idDoorBarricade*>(ent)->DoHack();
				break;
			}
		}
	}

	//Restart map 
	if (gameLocal.time > globalRestartTimer)
	{
		globalRestartTimer = gameLocal.time + 900000; //this is just so that we don't try to restart the map multiple times.
		//restart map.
		//idStr restartCommand = "reloadScript";
		//cmdSystem->BufferCommandText(CMD_EXEC_NOW, restartCommand.c_str());

		//BC 4-8-2025: map restart now uses the actual map restart logic (and not "reloadscript")
		idStr mapname;
		mapname = gameLocal.GetMapName();
		mapname.StripPath();
		mapname.StripFileExtension();
		mapname.StripPath();
		mapname.ToLower();

		if (stress_continueafterrestart.GetBool())
		{
			cmdSystem->BufferCommandText(CMD_EXEC_APPEND, va("map %s;\n savegameauto %s;\n stresstest;\n",mapname.c_str(), mapname.c_str()) );
		}
		else
		{
			sessLocal.MoveToNewMap(mapname.c_str());
		}
	}

	//Unlock all fuseboxes after a set time.
	if (!hasUnlockedGlobalFusebox && gameLocal.time > globalUnlockFuseboxTimer)
	{
		hasUnlockedGlobalFusebox = true;
		UnlockAllFusebox();
	}
}

void idStressTester::LoadRandomSave()
{
	saveloadTimer = gameLocal.random.RandomInt(stress_savetimer_min.GetInteger(), stress_savetimer_max.GetInteger());

	idStrList fileList;
	idList<fileTIME_T> fileTimes;
	sessLocal.GetSaveGameList(fileList, fileTimes);

	if (fileList.Num() <= 0)
	{
		common->Warning("Stresstest: savegame folder is empty...");
		return; //folder is empty, so do early exit here.
	}

	//randomly select a savegame.
	int randomSaveIndex = gameLocal.random.RandomInt(fileList.Num());
	common->Printf("Stresstest: loading '%s'\n", fileList[randomSaveIndex].c_str());

	idStr loadcommand = idStr::Format("loadGame %s", fileList[randomSaveIndex].c_str());
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, loadcommand.c_str());
}

void idStressTester::LoadLatestSave()
{
	idStrList fileList;
	idList<fileTIME_T> fileTimes;
	sessLocal.GetSaveGameList(fileList, fileTimes);

	if (fileList.Num() <= 0)
	{
		common->Warning("savegame folder is empty.");
		return; //folder is empty, so do early exit here.
	}

	idStrList loadGameList;
	loadGameList.Clear();
	loadGameList.SetNum(fileList.Num());

	loadGameList[0] = fileList[fileTimes[0].index]; //This sorts by date.

	//idStr savefilename = idStr::Format("savegames/%s.txt", fileList[i].c_str());
	sessLocal.LoadGame(loadGameList[0]);
	
}

void idStressTester::UpdateStuckLogic()
{
	if (gameLocal.time < stuckTimer)
		return;

	stuckTimer = gameLocal.time + 300;

	float distanceTravelled = (stuckLastPosition - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).Length();


	if (distanceTravelled < .1f)
	{
		//hasn't moved.
		stuckCounter++;

		if (stuckCounter >= STUCK_COUNTERTHRESHOLD)
		{
			cvarSystem->SetCVarBool("aas_randomPullPlayer", false);
			cvarSystem->SetCVarInteger("aas_pullPlayer", 0);
			
			stuckCounter = 0;
			state = POSTCRYO_DELAY;
			timer = 0;
		}
	}
	else
	{
		//reset the stuck counter.
		stuckCounter = 0;
	}
	

	stuckLastPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

}

void idStressTester::AttemptGetUp()
{
	if (!gameLocal.GetLocalPlayer()->GetFallenState())
		return;

	//Get up from fallen state.
	gameLocal.GetLocalPlayer()->Event_SetFallState(false, false, false);
}

void idStressTester::UnlockAllFusebox()
{
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->SetEnableTrashchutes(true);
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->SetEnableAirlocks(true);
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->SetEnableWindows(true);
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->SetEnableVents(true);
}

void idStressTester::AttemptJockeySlam()
{
	if (!meleeTarget.IsValid())
		return;

	if (!meleeTarget.GetEntity()->IsType(idAI::Type))
		return;

	static_cast<idAI*>(meleeTarget.GetEntity())->DoJockeyBrutalSlam();
	//gameLocal.GetLocalPlayer()->dojo
	//
	//if (aiEnt->jockeyAttackCurrentlyAvailable == JOCKATKTYPE_KILLENTITY)
	//	if (aiEnt->jockeyKillEntity.IsValid())
}

void idStressTester::AttemptJockey()
{
	//See if there's any enemies nearby.

	idEntity* enemy = FindVisibleEnemy();
	if (enemy == NULL)
		return;

	if (!enemy->IsType(idAI::Type))
		return;

	meleeTarget = enemy;
	gameLocal.GetLocalPlayer()->meleeTarget = static_cast<idAI*>(enemy);
	gameLocal.GetLocalPlayer()->SetJockeyMode(true);
}

idEntity* idStressTester::FindVisibleEnemy()
{
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || !entity->IsActive() || entity->health <= 0 || entity->IsHidden())
			continue;

		trace_t tr;
		gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), MASK_SOLID, NULL);
		if (tr.fraction >= 1)
			return entity;
	}

	return NULL;
}

void idStressTester::FrobDoorAttempt()
{
	trace_t tr;
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, gameLocal.GetLocalPlayer()->firstPersonViewOrigin + gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 64, MASK_SOLID, NULL);

	if (tr.fraction >= 1.0f)
		return;

	if (gameLocal.entities[tr.c.entityNum]->IsType(idWorldspawn::Type))
		return;

	if (gameLocal.entities[tr.c.entityNum]->IsType(idDoor::Type))
	{
		gameLocal.entities[tr.c.entityNum]->DoFrob(0, gameLocal.GetLocalPlayer());
	}
}

void idStressTester::ExitCryopod()
{
	//Player in cryo pod. Exit cryo pod.
	if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetPlayerExitedCryopod())
		return;

	//Find the cryo pod.
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idCryointerior::Type))
			continue;

		common->Printf("Stresstest: frobbing cryointerior.\n");
		static_cast<idCryointerior*>(ent)->DoFrob(PEEKFROB_INDEX, gameLocal.GetLocalPlayer());
		return;
	}	
}
