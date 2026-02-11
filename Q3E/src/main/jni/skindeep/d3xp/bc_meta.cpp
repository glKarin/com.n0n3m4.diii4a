


#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "Fx.h"

#include "Player.h"
#include "Actor.h"
#include "ai/AI.h"

#include "framework/DeclEntityDef.h"
#include "framework/FileSystem.h"
#include "WorldSpawn.h"

#include "idlib/LangDict.h"

#include "Game_local.h"


#include "Misc.h"
#include "bc_spearbot.h"
#include "bc_lostandfound.h"
#include "bc_vrvisor.h"
#include "bc_notewall.h"
#include "bc_tablet.h"
#include "bc_keypad.h"
#include "bc_maintpanel.h"
#include "SecurityCamera.h"
#include "Camera.h"
#include "BrittleFracture.h"
#include "bc_skullsaver.h"
#include "bc_infomap.h"
#include "bc_bossmonster.h"
#include "bc_ftl.h"
#include "bc_wallspeaker.h"
#include "bc_sabotagelever.h"
#include "bc_interestpoint.h"
#include "bc_repairbot.h"
#include "bc_dozerhatch.h"
#include "bc_manifeststation.h"
#include "bc_upgradecargo.h"
#include "bc_trigger_confinedarea.h"
#include "bc_repairpatrol.h"
#include "sw_skycontroller.h"
#include "bc_turret.h"
#include "bc_healthstation.h"
#include "bc_lifeboat.h"
#include "bc_camerasplice.h"
#include "bc_infostation.h"
#include "bc_airlock.h"
#include "bc_cryospawn.h"
#include "bc_ventdoor.h"
#include "bc_windowseal.h"
#include "bc_glasspiece.h"
#include "bc_gunner.h"
#include "bc_sabotageshutdown.h"
#include "bc_electricalbox.h"
#include "bc_pirateship.h"
#include "bc_catcage.h"
#include "bc_trashchute.h"
#include "bc_trashexit.h"
#include "bc_windowshutter.h"
#include "bc_idletask.h"
#include "bc_radiocheckin.h"
#include "bc_highlighter.h"
#include "bc_catpod_interior.h"

#include "bc_meta.h"



// ========================= AI =========================

const int ESCALATION_MAX = 1;

const int SEARCHNODE_LOS_DISTANCE_THRESHOLD = 64; //When doing LOS checks of searchnode-to-enemy, traceline endpos has to be XX close to be validly "close enough" to the observation point.
const int MONSTER_EYEHEIGHT = 63; //distance from floor to monster eyeball.


const int INTEREST_UPDATEINTERVAL = 300;

const int IDLETASK_UPDATEINTERVAL = 2000;


// ========================= BEACON =========================

const int BEACON_DISTANCE = 16000;
const int BEACON_VERTICALMAX = 8192;
const float BEACON_SPEED = .0002f;
const float BEACON_VERTICALSPEED = .00005f;


// ========================= REPAIRMANAGER =========================

const int REPAIRMANAGER_UPDATETIME = 3000;  //How often does the system check for things to repair
const int REPAIRMANAGER_REPAIRDELAY = 1000; //Guarantee an amount of time between a thing breaking and getting repaired. Because it feels weird when a broken thing is immediately repaired without any delay.
const int REPAIRMANAGER_REPAIR_VO_LINE_LENGTH = 3000;
const int REPAIRMANAGER_VO_COOLDOWNTIME = 30000;


//uncomment this to make the vent purge happen immediately
//#define DEBUG_FASTPURGE
#ifndef DEBUG_FASTPURGE
const int VENTPURGE_DISPATCHTIME = 3000;
const int VENTPURGE_ANNOUNCETIME = 4000; //milliseconds length of announcement wav file.
const int VENTPURGE_CHARGEUPTIME = 10000; //after countdown, do a chargeup sequence
#else
const int VENTPURGE_ANNOUNCETIME	= 350; //milliseconds length of announcement wav file.
const int VENTPURGE_CHARGEUPTIME	= 300; //after countdown, do a chargeup sequence
const int VENTPURGE_COUNTDOWNTIME	= 500; //how long the countdown is.
#endif

const int VENTPURGE_ALLCLEARANNOUNCE_DELAYTIME = 3000; //After purge happens, wait XX millseconds before doing the all-clear announcement.

const int ALLCLEAR_ANNOUNCE_VO_TIME = 3000; //time duration of the all-clear VO audio.




const int ESCALATIONMETER_MAX = 150;
const int ESCALATIONMETER_UPDATETIME = 50;
const int ESCALATIONMETER_INCREASEDELTA = 1;
const int ESCALATIONMETER_DECREASEDELTA = 3;


const int KILLCAM_DURATION = 400;
const int KILLCAM_DISTANCE = 72;
const int KILLCAM_HEIGHTOFFSET = 32;
const int KILLCAM_FOV = 100;
const int KILLCAM_THRESHOLDTIME = 300; //killcam activates if the time interval between final kill & victory conditions is less than XX amount of time.

const int COMBATSTATE_FAILSAFE_EXITTIME = 30000; //hard limit to how long the combat state can last.

const int MAXHEALTH_FTL_INCREASE = 3; //how much hitpoints to add after each FTL jump.


const int AI_AVOID_CRYOEXIT_ROOM_TIME = 60000; //avoid entering the cryo exit room XX milliseconds after player exits the cryospawn.

const int MAX_ITEMS_PER_PERSON = 1; //How many loot items are on a baddie.

const int REINFORCEMENTS_SPAWNDELAY = 15000; //after rescuing cats, how long before the reinforcement ship spawns in

const int SIGNALKIT_DEPLETION_CHECKINTERVAL = 7000;



const int SWORDFISH_INTERVALTIME = 30000;
const int SWORDFISH_SECURITY_MAXPERROOM = 3;


// --------------------------------- END CONSTANTS ---------------------------------

const idEventDef EV_Meta_GetBeaconPosition("GetBeaconPosition", NULL, 'v');
const idEventDef EV_Meta_GetLastSwordfishTime("GetLastSwordfishTime", NULL, 'f');
const idEventDef EV_Meta_SetSwordfishTime("SetSwordfishTime", "f");

const idEventDef EV_Meta_SetLKP("SetLKP", "dv");
const idEventDef EV_Meta_GetLKP("GetLKP", NULL, 'v');
const idEventDef EV_Meta_GetLKPReachable("GetLKPReachable", NULL, 'v');

const idEventDef EV_Meta_GetEscalationLevel("GetEscalationLevel", NULL, 'd');
const idEventDef EV_Meta_GetSearchTimer("GetSearchTimer", NULL, 'f');
const idEventDef EV_Meta_SomeoneStartedSearching("SomeoneStartedSearching");

const idEventDef EV_Meta_GetSignallampActive("signallampActive", NULL, 'd');

const idEventDef EV_Meta_SetCombatState("SetMetaCombatState", "d");
const idEventDef EV_Meta_GetCombatState("GetMetaCombatState", NULL, 'd');
const idEventDef EV_Meta_startpostgame("StartPostgame");

const idEventDef EV_Meta_GetAliveEnemies("GetAliveEnemies", NULL, 'd');

const idEventDef EV_Meta_SetWorldBaffled("SetWorldBaffled", "d");
const idEventDef EV_Meta_StartGlobalStunState("StartGlobalStunState", "s");

const idEventDef EV_Meta_LaunchScriptedProjectile("launchScriptedProjectile", "esvv", 'e');
const idEventDef EV_Meta_SetDebrisBurst("setDebrisBurst", "svdffv");


const idEventDef EV_Meta_SetEnableTrash("SetEnableTrash", "d");
const idEventDef EV_Meta_SetEnableAirlocks("SetEnableAirlocks", "d");
const idEventDef EV_Meta_SetEnableVents("SetEnableVents", "d");
const idEventDef EV_Meta_SetEnableWindows("SetEnableWindows", "d");

const idEventDef EV_Meta_EnableRadiocheckin("EnableRadiocheckin", "d");


const idEventDef EV_Meta_SetMilestone("SetMilestone", "d");

const idEventDef EV_Meta_SetExitVR("SetExitVR");

const idEventDef EV_Meta_DoHighlighter("doHighlighter", "ee");



CLASS_DECLARATION(idTarget, idMeta)
EVENT(EV_PostSpawn, idMeta::Event_PostSpawn)

EVENT(EV_Meta_GetBeaconPosition, idMeta::Event_GetBeaconPosition)
EVENT(EV_Meta_GetLastSwordfishTime, idMeta::Event_GetLastSwordfishTime)
EVENT(EV_Meta_SetSwordfishTime, idMeta::Event_SetSwordfishTime)

EVENT(EV_Meta_SetLKP, idMeta::SetLKPVisible)
EVENT(EV_Meta_GetLKP, idMeta::GetLKPPosition)
EVENT(EV_Meta_GetLKPReachable, idMeta::Event_GetLKPReachable)
EVENT(EV_Meta_GetEscalationLevel, idMeta::Event_GetEscalationLevel)
EVENT(EV_Meta_GetSearchTimer, idMeta::Event_GetSearchTimer)
EVENT(EV_Meta_SomeoneStartedSearching, idMeta::SomeoneStartedSearching)
EVENT(EV_Meta_GetSignallampActive, idMeta::GetSignallampActive)
EVENT(EV_Meta_SetCombatState, idMeta::Event_SetCombatState)
EVENT(EV_Meta_GetCombatState, idMeta::Event_GetCombatState)
EVENT(EV_Meta_startpostgame, idMeta::StartPostGame)
EVENT(EV_Meta_GetAliveEnemies, idMeta::Event_GetAliveEnemies)
EVENT(EV_Meta_SetWorldBaffled, idMeta::SetWorldBaffled)
EVENT(EV_Meta_StartGlobalStunState, idMeta::Event_StartGlobalStunState)
EVENT(EV_Meta_LaunchScriptedProjectile, idMeta::Event_LaunchScriptedProjectile)
EVENT(EV_Meta_SetDebrisBurst, idMeta::Event_SetDebrisBurst)

EVENT(EV_Meta_SetEnableTrash, idMeta::SetEnableTrashchutes)
EVENT(EV_Meta_SetEnableAirlocks, idMeta::SetEnableAirlocks)
EVENT(EV_Meta_SetEnableVents, idMeta::SetEnableVents)
EVENT(EV_Meta_SetEnableWindows, idMeta::SetEnableWindows)

EVENT(EV_Meta_EnableRadiocheckin, idMeta::SetEnableRadioCheckin)

EVENT(EV_Meta_SetMilestone, idMeta::SetMilestone)
EVENT(EV_Meta_SetExitVR, idMeta::SetExitVR)
EVENT(EV_Meta_DoHighlighter, idMeta::Event_DoHighlighter)



END_CLASS

idMeta::idMeta()
{
	nodePosArraySize = 0;
	for (int i = 0; i < 16; i++)
	{
		nodePositions[0] = vec3_zero;
	}
	lastNodeSearchPos = idVec3(-1, -1, -1);

	memset(pipeStatuses, 0, sizeof(bool)*MAX_PIPESTATUSES);
	memset(mapguiBound, 0, sizeof(idVec2)*2);
	memset(milestoneStartArray, 0, sizeof(bool)*MAXMILESTONES);

	radioCheckinEnt = nullptr;
	highlighterEnt = nullptr;

	lkpEnt = nullptr;
	lkpEnt_Eye = nullptr;

	killcamCamera = nullptr;
	lkpLocbox = nullptr;

	beamLure = nullptr;
	beamLureTarget = nullptr;
	beamRectangle = nullptr;

	combatMetastate = COMBATSTATE_IDLE;
	lastCombatMetastate = COMBATSTATE_IDLE;
	escalationLevel = 0;
	lastSwordfishTime = 0;
	searchTimer = 0;
	lkpReachablePosition = vec3_zero;
	repairmanagerTimer = 0;
    repairVO_timer = 0;

	ventpurgeState = VENTPURGESTATE_IDLE;
	ventpurgeTimer = 0;

	escalationmeterAmount = 0;
	escalationmeterTimer = 0;

	idletaskTimer = 0;

	enemydestroyedCheckState = ENEMYDESTROYEDSTATECHECK_CATS_AWAITING_RESCUE;
	enemydestroyedCheckTimer = 0;

	playerHasExitedCryopod = false;
	cryoExitLocationEntnum = -1;
	cryoExitTimer = 0;

	enemiesEliminatedInCurrentPhase = 0; //in the current 'phase' (time between previous FTL jump & upcoming FTL jump), how much enemies were eliminated.

	allclearState = ALLCLEAR_DORMANT;
	combatstateDurationTime = 0;

	highlighterEnt = NULL;

	totalCatCagesInLevel = 0;
	catpodInterior = NULL;

	playerIsInSpace = false;
	playerSpaceTimerStart = 0;
	playerSpaceTimerTotal = 0;

	playerHasEnteredCatpod = false;

	enemiesKnowOfNina = false;

	signalkitDepletionTimer = 5000;
	signalkitDepletionCheckActive = true;

	swordfishTimer = 0;
	dispatchVoiceprint = VOICEPRINT_A;
}

void idMeta::Spawn()
{
    idDict		args;

    //spawn LKP base model.
    args.Clear();
    args.SetInt("solid", 0);
    args.Set("model", "models/objects/ui_lkp/lkp.ase");
    args.Set("snd_lkp_spawn", "lkp_spawn");
    args.SetBool("spin", true);
    lkpEnt = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
    lkpEnt->Hide();

    //spawn LKP eyeball.
    args.Clear();
    args.SetInt("solid", 0);
    args.Set("model", "models/objects/ui_lkp/lkp_eye.ase");
	args.SetBool("drawGlobally", true);
    lkpEnt_Eye = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
    lkpEnt_Eye->Bind(lkpEnt, false);
    lkpEnt_Eye->Hide();


	//BC 3-24-2025: lkp locbox
	#define LOCBOXRADIUS 8
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_def_gameplay_lkp"));
	args.SetVector("origin", idVec3(0, 0, 49));
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	args.SetFloat("locboxDistScale", 10.0f); //BC 4-3-2025: make lkp locbox range larger
	lkpLocbox = static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));
	if (lkpLocbox)
	{
		lkpLocbox->Bind(lkpEnt, false);
		lkpLocbox->Hide();
	}


    totalEnemies = 0;
	
	metaState = METASTATE_ACTIVE;
	killcamTimer = 0;
	killcamTarget = NULL;
	killcamCamera = NULL;
	lastKilltime = 0;
	combatstateStartTime = 0;
	beaconAngle = 0;
	walkietalkieLureTask = NULL;


	//Laser endpoint.
	args.Clear();
	args.SetBool("drawGlobally", true);
	beamLureTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	//beamLureTarget->BecomeActive(TH_PHYSICS);
	beamLureTarget->SetOrigin(GetPhysics()->GetOrigin());

	//Laser startpoint.
	args.Clear();
	args.Set("target", beamLureTarget->name.c_str());
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetBool("start_off", false);
	args.Set("width", spawnArgs.GetString("laserwidth", "2"));
	args.Set("skin", spawnArgs.GetString("laserskin", "skins/beam_lure"));
	args.SetBool("drawGlobally", true);
	beamLure = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	beamLure->Hide();
	
	args.Clear();
	args.SetInt("solid", 0);
	args.Set("model", "models/objects/ui_person/ui_person.ase");
	args.Set("skin", "skins/ui_person/rectangle");
	args.SetBool("drawGlobally", true);
	beamRectangle = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	beamRectangle->Hide();
	

	//lastFTLActiveState = FTL_ACTIVE; //was the FTL on in the last frame
	ftlGUIsAreOn = true;

	glassdestroyTimer = 0;



	//Spawn radio checkin ent.
	if (1)
	{
		radioCheckinEnt = NULL;

		const idDeclEntityDef *radioDef;
		radioDef = gameLocal.FindEntityDef("func_radiocheckin", false);
		if (!radioDef)
		{
			gameLocal.Error("idMeta failed to find idRadioCheckin definition.");
		}

		idEntity *tempEnt;
		gameLocal.SpawnEntityDef(radioDef->dict, &tempEnt, false);
		radioCheckinEnt = static_cast<idRadioCheckin*>(tempEnt);
		if (!radioCheckinEnt)
		{
			gameLocal.Error("idMeta failed to spawn idRadioCheckin.");
		}
	}

	//Spawn highlighter ent.
	if (1)
	{
		const idDeclEntityDef* highlighterDef;
		highlighterDef = gameLocal.FindEntityDef("target_highlighter", false);
		if (!highlighterDef)
		{
			gameLocal.Error("idMeta failed to find idHighlighter definition.");
			return;
		}

		idEntity* tempEnt;
		gameLocal.SpawnEntityDef(highlighterDef->dict, &tempEnt, false);
		highlighterEnt = static_cast<idHighlighter*>(tempEnt);
		if (!highlighterEnt)
		{
			gameLocal.Error("idMeta failed to spawn idHighlighter.");
		}
	}

	enemyThinksPlayerIsInVentOrAirlock = true;

	isWorldBaffled = false;

	PostEventMS(&EV_PostSpawn, 0);

	reinforcementPhase = REINF_NONE;
	reinforcementPhaseTimer = 0;


	


#if _DEBUG
	VerifyLevelIndexes();
#endif

	//spawn end.
}

//Called after everything has spawned into world.
void idMeta::Event_PostSpawn(void)
{
	int amountOfRepairbotSpawners = 0;
	int amountOfRepairPatrolpoints = 0;

	bool foundDoorframe = 0;
	int hasDoorframe_b = false;

	bool hasCryospawn = false;

	//Find the entities that we need to track. Store them in a pointer.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (gameLocal.entities[i]->entityNumber == this->entityNumber)
			continue;

		if (gameLocal.entities[i]->IsType(idFTL::Type))
		{
			GetFTLDrive = gameLocal.entities[i];
			common->Printf("idMeta: found FTL drive '%s'\n", gameLocal.entities[i]->GetName());
		}

		if (gameLocal.entities[i]->IsType(idBossMonster::Type))
		{
			bossEnt = gameLocal.entities[i];
			common->Printf("idMeta: found Boss '%s'\n", gameLocal.entities[i]->GetName());
		}

		if (gameLocal.entities[i]->IsType(idBrittleFracture::Type))
		{
			static_cast<idBrittleFracture*>(gameLocal.entities[i])->SetRoomAssignment(); //This is where brittlefracture entities are told what room they're assigned to.
		}

		if (gameLocal.entities[i]->IsType(idDozerhatch::Type))
		{
			amountOfRepairbotSpawners++;
		}

		if (gameLocal.entities[i]->IsType(idRepairPatrolNode::Type))
		{
			amountOfRepairPatrolpoints++;
		}

		if (gameLocal.entities[i]->IsType(idSkyController::Type))
		{
			skyController = gameLocal.entities[i];
			static_cast<idSkyController*>(skyController.GetEntity())->Init();
		}

		if (gameLocal.entities[i]->IsType(idCryospawn::Type))
		{
			//So,there SHOULD only be one cryo spawn in the level now, because all the others will have gotten deleted during idPlayer::DoCryoSpawnLogic().			

			idLocationEntity* locationEnt = gameLocal.LocationForEntity(gameLocal.entities[i]);
			if (locationEnt)
			{
				//Tell idMeta the entnum of the location entity that the cryopod exit belongs to. This is so that we can tell the AI to avoid patrolling into thsi room at game start.
				cryoExitLocationEntnum = locationEnt->entityNumber;
			}

			hasCryospawn = true;
		}

		if (!doorframeInfoEnt.IsValid() && !foundDoorframe)
		{
			if (gameLocal.entities[i]->IsType(idStaticEntity::Type))
			{
				if (idStr::FindText(gameLocal.entities[i]->spawnArgs.GetString("model"), "frame_b.ase", false) >= 0)
				{
					hasDoorframe_b = true;

					if (idStr::FindText(gameLocal.entities[i]->spawnArgs.GetString("gui"), "doorframe_infopanel.gui", false) >= 0)
					{
						//Found an entity with the gui we're looking for. These guis are all synced, so we only need this one.
						doorframeInfoEnt = gameLocal.entities[i];
						foundDoorframe = true;
					}
				}
			}
		}

		if (gameLocal.entities[i]->IsType(idCatpodInterior::Type))
		{
			catpodInterior = gameLocal.entities[i];
		}
	}

	if (!hasCryospawn)
	{
		playerHasExitedCryopod = true; //For debug purposes: if there is no cryospawn in level, then we just go forward with the concept that the player has exited the cryospawn.
	}


	//Spawn enemies from the info_enemyspawnpoint points.
	if (!SpawnEnemies(cryoExitLocationEntnum, true))
	{
		gameLocal.Warning("SpawnEnemies() failed. Trying fallback (skipping the cryospawn check).");
		if (SpawnEnemies(cryoExitLocationEntnum, false))
		{
			gameLocal.Warning("SpawnEnemies() fallback successful.\n");
		}
		else
		{
			gameLocal.Error("SpawnEnemies() fallback failed.");
		}
	}


	if (!foundDoorframe && hasDoorframe_b)
	{
		gameLocal.Warning("doorframes are missing guis/game/doorframe_infopanel.gui");
	}

	if (amountOfRepairbotSpawners > 0 && amountOfRepairPatrolpoints <= 0)
	{
		gameLocal.Warning("idMeta: level has repairbot hatches but does not have 'ai_repairpatrolnode' patrol nodes.\n");
	}


	for (int i = 0; i < MAX_PIPESTATUSES; i++)
	{
		pipeStatuses[i] = true; //everything is operational by default.
	}

	//Sanity checks. We use warnings instead of errors because we still want to retain test map functionality.
	//if (!GetFTLDrive.IsValid())			{ gameLocal.Warning("idMeta failed to find FTL drive."); }
	//if (!bossEnt.IsValid())				{ gameLocal.Warning("idMeta failed to find boss actor."); }
	//if (!skyController.IsValid())		{ gameLocal.Warning("idMeta failed to find sky controller. Sky will not be dynamic."); }
	if (!GetFTLDrive.IsValid())
	{
		common->Printf("No FTL entities found.\n");
	}

	currentNavnode = 0;
	nextNavnode = 0;
	SetSpaceNode(NAVNODE_0);

	EquipEnemyLoadout();

	InitializeUpgradeCargos();
	DoUpgradeCargoUnlocks(); //level start cargo unlocks.


	InitializeItemSpawns();

	SetupWindowseals();

	//Get total enemy count.    
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idActor::Type) || entity->team != TEAM_ENEMY)
			continue;

		totalEnemies++;
	}


	//tell the radio checkin system to label each baddie with a unit designation (alpha, bravo, etc)
	radioCheckinEnt->InitializeEnemyCount();


	//Camera splice setup.
	PopulateCameraSplices();


	//Set up the kill tally HUD element.
	//int totalEnemies = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetTotalEnemyCount();
	//gameLocal.GetLocalPlayer()->SetHudNamedEvent(idStr::Format("totalenemies%d", totalEnemies));
	SetupMapBounds();


	gameLocal.GetLocalPlayer()->hud->SetStateInt("combatstate", -1);


	NameTheEnemies();


	totalCatCagesInLevel = CalculateTotalCatCages();


	//StressTester spawn logic.
	if (gameLocal.world->spawnArgs.GetBool("stresstest", "0"))
	{
		const idDeclEntityDef* stressDef;
		stressDef = gameLocal.FindEntityDef("target_stresstester", false);
		if (stressDef)
		{
			common->Printf("** Stresstest spawnarg on **\n");
			idEntity* stressEnt;
			gameLocal.SpawnEntityDef(stressDef->dict, &stressEnt, false);
		}
	}


	//Player wristmap fusebox display.
	gameLocal.GetLocalPlayer()->SetArmstatsFuseboxNeedUpdate();


	if (gameLocal.world->spawnArgs.GetBool("keypad_random", "0"))
	{
		GenerateKeypadcodes();
	}


	InitializeMilestoneStartArray();


	//Do we do the check whether signal kits have been depleted
	signalkitDepletionCheckActive = gameLocal.world->spawnArgs.GetBool("signalkit_check", "1");



#if _DEBUG
	//autoexec line in worldspawn. Note: this is only enabled in debug builds.
	idStr autoexecLine = gameLocal.world->spawnArgs.GetString("autoexec");
	if (autoexecLine.Length() > 0)
	{
		common->Printf("\n******** RUNNING WORLDSPAWN AUTOEXEC ********:\n'%s'\n\n", autoexecLine.c_str());
		cmdSystem->BufferCommandText(CMD_EXEC_NOW, autoexecLine.c_str());
	}
#endif


#ifdef DEMO
	//Make the emails become forgotten, so that the game can continually be restarted in a demo booth setting.
	int count = declManager->GetNumDecls(DECL_PDA); //total email count.
	for (int i = 0; i < count; i++)
	{
		const idDeclPDA* pda = static_cast<const idDeclPDA*>(declManager->DeclByIndex(DECL_PDA, i));
		pda->ResetEmails();
	}

	common->Printf("--------------------\n\n\n\nDEBUG EMAIL CLEAR\n\n\n\n--------------------\n");
#endif
}

void idMeta:: Save(idSaveGame* savefile) const
{
	savefile->WriteVec3( beaconPosition ); //  idVec3 beaconPosition
	savefile->WriteObject( GetFTLDrive ); //  idEntityPtr<idEntity> GetFTLDrive

	savefile->WriteObject( lkpEnt ); // idEntity *lkpEnt

	savefile->WriteObject( bossEnt ); //  idEntityPtr<idEntity> bossEnt
	savefile->WriteObject( skyController ); //  idEntityPtr<idEntity> skyController

	SaveFileWriteArray( nodePositions, MAX_LOS_NODES, WriteVec3 ); // idVec3 nodePositions[MAX_LOS_NODES]

	savefile->WriteInt( nodePosArraySize ); //  int nodePosArraySize
	savefile->WriteVec3( lastNodeSearchPos ); //  idVec3 lastNodeSearchPos

	savefile->WriteInt( combatMetastate ); //  int combatMetastate

	savefile->WriteObject( signallampEntity ); //  idEntityPtr<idEntity> signallampEntity

	savefile->WriteInt( itemdropsTable.Num()) ; //  idHashTable<idStrList> itemdropsTable
	for (int idx = 0; idx < itemdropsTable.Num(); idx++) {
		idStr outKey;
		idStrList outVal;
		itemdropsTable.GetIndex(idx, &outKey, &outVal);
		savefile->WriteString( outKey );
		savefile->WriteListString( outVal );
	}

	savefile->WriteInt( totalSecuritycamerasAtGameStart ); //  int totalSecuritycamerasAtGameStart

	savefile->WriteInt( dispatchVoiceprint ); //  int dispatchVoiceprint

	savefile->WriteObject( radioCheckinEnt ); // class idRadioCheckin* radioCheckinEnt
	savefile->WriteObject( highlighterEnt ); // class idHighlighter* highlighterEnt

	savefile->WriteInt( reinforcementPhase ); //  int reinforcementPhase
	savefile->WriteInt( reinforcementPhaseTimer ); //  int reinforcementPhaseTimer

	savefile->WriteFloat( beaconAngle ); //  float beaconAngle

	savefile->WriteFloat( lastSwordfishTime ); //  float lastSwordfishTime

	SaveFileWriteArray(pipeStatuses, MAX_PIPESTATUSES, WriteBool); // bool pipeStatuses[MAX_PIPESTATUSES]

	savefile->WriteObject( lkpEnt_Eye );

	savefile->WriteVec3( lkpReachablePosition ); //  idVec3 lkpReachablePosition

	savefile->WriteInt( escalationLevel ); //  int escalationLevel

	savefile->WriteInt( currentNavnode ); //  int currentNavnode
	savefile->WriteInt( nextNavnode ); //  int nextNavnode

	savefile->WriteInt( searchTimer ); //  int searchTimer

	savefile->WriteInt( lastCombatMetastate ); //  int lastCombatMetastate

	savefile->WriteInt( repairmanagerTimer ); //  int repairmanagerTimer
	savefile->WriteInt( repairVO_timer ); //  int repairVO_timer

	savefile->WriteInt( ventpurgeState ); //  int ventpurgeState
	savefile->WriteInt( ventpurgeTimer ); //  int ventpurgeTimer
	savefile->WriteInt( allclearState ); //  int allclearState
	savefile->WriteInt( allclearTimer ); //  int allclearTimer

	savefile->WriteInt( escalationmeterAmount ); //  int escalationmeterAmount
	savefile->WriteInt( escalationmeterTimer ); //  int escalationmeterTimer

	savefile->WriteInt( idletaskTimer ); //  int idletaskTimer

	savefile->WriteInt( totalEnemies ); //  int totalEnemies

	savefile->WriteInt( metaState ); //  int metaState

	savefile->WriteInt( enemydestroyedCheckTimer ); //  int enemydestroyedCheckTimer
	savefile->WriteInt( enemydestroyedCheckState ); //  int enemydestroyedCheckState

	savefile->WriteInt( killcamTimer ); //  int killcamTimer
	savefile->WriteObject( killcamTarget ); //  idEntityPtr<idEntity> killcamTarget
	savefile->WriteObject( killcamCamera ); //  idEntity * killcamCamera
	savefile->WriteInt( lastKilltime ); //  int lastKilltime

	savefile->WriteInt( combatstateStartTime ); //  int combatstateStartTime

	SaveFileWriteArray(mapguiBound, 2, WriteVec2); // idVec2 mapguiBound[2]

	savefile->WriteInt( cryoExitLocationEntnum ); //  int cryoExitLocationEntnum
	savefile->WriteBool( playerHasExitedCryopod ); //  bool playerHasExitedCryopod
	savefile->WriteInt( cryoExitTimer ); //  int cryoExitTimer

	savefile->WriteInt( enemiesEliminatedInCurrentPhase ); //  int enemiesEliminatedInCurrentPhase

	savefile->WriteObject( walkietalkieLureTask ); //  idEntityPtr<idEntity> walkietalkieLureTask
	savefile->WriteObject( beamLure ); //  idBeam* beamLure
	savefile->WriteObject( beamLureTarget ); //  idBeam* beamLureTarget
	savefile->WriteObject( beamRectangle ); //  idEntity* beamRectangle

	savefile->WriteObject( doorframeInfoEnt ); //  idEntityPtr<idEntity> doorframeInfoEnt
	savefile->WriteBool( ftlGUIsAreOn ); //  bool ftlGUIsAreOn

	savefile->WriteInt( glassdestroyTimer ); //  int glassdestroyTimer

	savefile->WriteBool( enemyThinksPlayerIsInVentOrAirlock ); //  bool enemyThinksPlayerIsInVentOrAirlock

	savefile->WriteBool( isWorldBaffled ); //  bool isWorldBaffled

	savefile->WriteInt( combatstateDurationTime ); //  int combatstateDurationTime

	savefile->WriteInt( totalCatCagesInLevel ); //  int totalCatCagesInLevel

	savefile->WriteObject( catpodInterior ); //  idEntityPtr<idEntity> catpodInterior

	SaveFileWriteArray(milestoneStartArray, MAXMILESTONES, WriteBool); // bool milestoneStartArray[MAXMILESTONES]

	savefile->WriteBool( playerIsInSpace ); //  bool playerIsInSpace
	savefile->WriteInt( playerSpaceTimerStart ); //  int playerSpaceTimerStart
	savefile->WriteInt( playerSpaceTimerTotal ); //  int playerSpaceTimerTotal

	savefile->WriteBool( playerHasEnteredCatpod ); //  bool playerHasEnteredCatpod

	savefile->WriteBool( enemiesKnowOfNina ); //  bool enemiesKnowOfNina

	savefile->WriteInt( signalkitDepletionTimer ); //  int signalkitDepletionTimer

	savefile->WriteBool( signalkitDepletionCheckActive ); //  bool signalkitDepletionCheckActive

	savefile->WriteInt( swordfishTimer ); //  int swordfishTimer

	savefile->WriteObject( lkpLocbox ); // idEntity* lkpLocbox
}

void idMeta::Restore(idRestoreGame* savefile)
{
	savefile->ReadVec3( beaconPosition ); //  idVec3 beaconPosition
	savefile->ReadObject( GetFTLDrive ); //  idEntityPtr<idEntity> GetFTLDrive

	savefile->ReadObject( lkpEnt ); // idEntity *lkpEnt

	savefile->ReadObject( bossEnt ); //  idEntityPtr<idEntity> bossEnt
	savefile->ReadObject( skyController ); //  idEntityPtr<idEntity> skyController

	SaveFileReadArray( nodePositions, ReadVec3 ); // idVec3 nodePositions[MAX_LOS_NODES]

	savefile->ReadInt( nodePosArraySize ); //  int nodePosArraySize
	savefile->ReadVec3( lastNodeSearchPos ); //  idVec3 lastNodeSearchPos

	savefile->ReadInt( combatMetastate ); //  int combatMetastate

	savefile->ReadObject( signallampEntity ); //  idEntityPtr<idEntity> signallampEntity

	int num;
	savefile->ReadInt( num ); //  idHashTable<idStrList> itemdropsTable
	for (int idx = 0; idx < num; idx++) {
		idStr outKey;
		idStrList outVal;
		savefile->ReadString( outKey );
		savefile->ReadListString( outVal );
		itemdropsTable.Set( outKey, outVal );
	}

	savefile->ReadInt( totalSecuritycamerasAtGameStart ); //  int totalSecuritycamerasAtGameStart

	savefile->ReadInt( dispatchVoiceprint ); //  int dispatchVoiceprint

	savefile->ReadObject( CastClassPtrRef( radioCheckinEnt ) ); // class idRadioCheckin* radioCheckinEnt
	savefile->ReadObject( CastClassPtrRef( highlighterEnt ) ); // class idHighlighter* highlighterEnt

	savefile->ReadInt( reinforcementPhase ); //  int reinforcementPhase
	savefile->ReadInt( reinforcementPhaseTimer ); //  int reinforcementPhaseTimer

	savefile->ReadFloat( beaconAngle ); //  float beaconAngle

	savefile->ReadFloat( lastSwordfishTime ); //  float lastSwordfishTime

	SaveFileReadArray(pipeStatuses, ReadBool);  // bool pipeStatuses[MAX_PIPESTATUSES]

	savefile->ReadObject( lkpEnt_Eye );

	savefile->ReadVec3( lkpReachablePosition ); //  idVec3 lkpReachablePosition

	savefile->ReadInt( escalationLevel ); //  int escalationLevel

	savefile->ReadInt( currentNavnode ); //  int currentNavnode
	savefile->ReadInt( nextNavnode ); //  int nextNavnode

	savefile->ReadInt( searchTimer ); //  int searchTimer

	savefile->ReadInt( lastCombatMetastate ); //  int lastCombatMetastate

	savefile->ReadInt( repairmanagerTimer ); //  int repairmanagerTimer
	savefile->ReadInt( repairVO_timer ); //  int repairVO_timer

	savefile->ReadInt( ventpurgeState ); //  int ventpurgeState
	savefile->ReadInt( ventpurgeTimer ); //  int ventpurgeTimer
	savefile->ReadInt( allclearState ); //  int allclearState
	savefile->ReadInt( allclearTimer ); //  int allclearTimer

	savefile->ReadInt( escalationmeterAmount ); //  int escalationmeterAmount
	savefile->ReadInt( escalationmeterTimer ); //  int escalationmeterTimer

	savefile->ReadInt( idletaskTimer ); //  int idletaskTimer

	savefile->ReadInt( totalEnemies ); //  int totalEnemies

	savefile->ReadInt( metaState ); //  int metaState

	savefile->ReadInt( enemydestroyedCheckTimer ); //  int enemydestroyedCheckTimer
	savefile->ReadInt( enemydestroyedCheckState ); //  int enemydestroyedCheckState

	savefile->ReadInt( killcamTimer ); //  int killcamTimer
	savefile->ReadObject( killcamTarget ); //  idEntityPtr<idEntity> killcamTarget
	savefile->ReadObject( killcamCamera ); //  idEntity * killcamCamera
	savefile->ReadInt( lastKilltime ); //  int lastKilltime

	savefile->ReadInt( combatstateStartTime ); //  int combatstateStartTime

	SaveFileReadArray(mapguiBound, ReadVec2); // idVec2 mapguiBound[2]

	savefile->ReadInt( cryoExitLocationEntnum ); //  int cryoExitLocationEntnum
	savefile->ReadBool( playerHasExitedCryopod ); //  bool playerHasExitedCryopod
	savefile->ReadInt( cryoExitTimer ); //  int cryoExitTimer

	savefile->ReadInt( enemiesEliminatedInCurrentPhase ); //  int enemiesEliminatedInCurrentPhase

	savefile->ReadObject( walkietalkieLureTask ); //  idEntityPtr<idEntity> walkietalkieLureTask
	savefile->ReadObject( CastClassPtrRef( beamLure ) ); //  idBeam* beamLure
	savefile->ReadObject( CastClassPtrRef( beamLureTarget ) ); //  idBeam* beamLureTarget
	savefile->ReadObject( beamRectangle ); //  idEntity* beamRectangle

	savefile->ReadObject( doorframeInfoEnt ); //  idEntityPtr<idEntity> doorframeInfoEnt
	savefile->ReadBool( ftlGUIsAreOn ); //  bool ftlGUIsAreOn

	savefile->ReadInt( glassdestroyTimer ); //  int glassdestroyTimer

	savefile->ReadBool( enemyThinksPlayerIsInVentOrAirlock ); //  bool enemyThinksPlayerIsInVentOrAirlock

	savefile->ReadBool( isWorldBaffled ); //  bool isWorldBaffled

	savefile->ReadInt( combatstateDurationTime ); //  int combatstateDurationTime

	savefile->ReadInt( totalCatCagesInLevel ); //  int totalCatCagesInLevel

	savefile->ReadObject( catpodInterior ); //  idEntityPtr<idEntity> catpodInterior

	SaveFileReadArray(milestoneStartArray, ReadBool); // bool milestoneStartArray[MAXMILESTONES]

	savefile->ReadBool( playerIsInSpace ); //  bool playerIsInSpace
	savefile->ReadInt( playerSpaceTimerStart ); //  int playerSpaceTimerStart
	savefile->ReadInt( playerSpaceTimerTotal ); //  int playerSpaceTimerTotal

	savefile->ReadBool( playerHasEnteredCatpod ); //  bool playerHasEnteredCatpod

	savefile->ReadBool( enemiesKnowOfNina ); //  bool enemiesKnowOfNina

	savefile->ReadInt( signalkitDepletionTimer ); //  int signalkitDepletionTimer

	savefile->ReadBool( signalkitDepletionCheckActive ); //  bool signalkitDepletionCheckActive

	savefile->ReadInt( swordfishTimer ); //  int swordfishTimer

	savefile->ReadObject( lkpLocbox ); // idEntity* lkpLocbox
}


void idMeta::VerifyLevelIndexes()
{
	idList<int> indexList;

	int num = declManager->GetNumDecls(DECL_MAPDEF);
	for (int i = 0; i < num; i++)
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_MAPDEF, i));
		if (!mapDef)
			continue;
		
		int levelindex = mapDef->dict.GetInt("levelindex", "-1");

		if (levelindex <= -1)
			continue;

		if (indexList.Find(levelindex))
		{
			gameLocal.Error("maps.def: level index '%d' is used more than once.\n", levelindex);
			return;
		}			

		indexList.Append(levelindex);		
	}

	//Ok, there are NO duplicates. That's great.

	//Now check if level index list is contiguous.
	indexList.Sort();
	for (int i = 0; i < indexList.Num(); i++)
	{
		int curValue = indexList[i];
		if (curValue != (i + 1))
		{
			gameLocal.Error("maps.def: missing level index value '%d'\n", i + 1);
			return;
		}
	}

	//Ok, there are no gaps, level index list is contiguous. Fantastic.
}





//Set up the milestone array. This is so that we know which milestones were achieved in this current run. This info is used
//for the UI fanfare during the spectator cam.
void idMeta::InitializeMilestoneStartArray()
{
	int levelIndex = -1;
	idStr mapname;
	mapname = gameLocal.GetMapName();
	mapname.StripPath();
	mapname.StripFileExtension();
	mapname.ToLower();

	//Grab map def of current map.
	const idDecl* mapDecl = declManager->FindType(DECL_MAPDEF, mapname, false);
	const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(mapDecl);
	if (mapDef)
	{
		//found the map def.
		levelIndex = mapDef->dict.GetInt("levelindex");
	}

	if (levelIndex < 0)
	{
		//gameLocal.Warning("InitializeMilestoneStartArray: failed to find levelIndex for '%s'. Bad maps.def setup?\n", mapname.c_str());
		return;
	}

	for (int i = 0; i < MAXMILESTONES; i++)
	{
		idStr milestoneName = gameLocal.GetMilestonenameViaLevelIndex(levelIndex, i);
		bool wasMilestoneAlreadyEarned = gameLocal.persistentLevelInfo.GetBool(milestoneName.c_str());
		milestoneStartArray[i] = wasMilestoneAlreadyEarned;
	}
}

bool idMeta::WasMilestoneTrueAtLevelStart(int milestoneIndex)
{
	return milestoneStartArray[milestoneIndex];
}

void idMeta::SetupMapBounds()
{
	mapguiBound[0] = vec2_zero;
	mapguiBound[1] = vec2_zero;

	//Set up the map bounds for the infostations.
	const char *topleftName = gameLocal.world->spawnArgs.GetString("map_topleft");
	const char *bottomrightName = gameLocal.world->spawnArgs.GetString("map_bottomright");

	if (topleftName[0] == '\0' || bottomrightName[0] == '\0')
		return;

	idEntity *topleftEnt = gameLocal.FindEntity(topleftName);	
	if (topleftEnt == NULL)
	{
		gameLocal.Error("IDMETA:MAPBOUNDS: Couldn't find map_topleft target_null: '%s'", topleftName);
	}

	idEntity *bottomrightEnt = gameLocal.FindEntity(bottomrightName);
	if (bottomrightEnt == NULL)
	{
		gameLocal.Error("IDMETA:MAPBOUNDS: Couldn't find map_bottomright target_null: '%s'", bottomrightName);
	}

	//Sanity check
	idVec3 topleftPos = topleftEnt->GetPhysics()->GetOrigin();
	idVec3 bottomrightPos = bottomrightEnt->GetPhysics()->GetOrigin();
	if (bottomrightPos.x < topleftPos.x || bottomrightPos.y > topleftPos.y)
	{
		gameLocal.Error("IDMETA:MAPBOUNDS: map_topleft isn't to the top and left of the map_bottomright target_null.");
	}

	//Make sure it's a square.
	int width = bottomrightPos.x - topleftPos.x;
	int height = topleftPos.y - bottomrightPos.y;
	if (width != height)
	{
		int delta;
		if (width > height)
			delta = width - height;
		else
			delta = height - width;

		gameLocal.Error("IDMETA:MAPBOUNDS: map_topleft & map_bottomright need to form a square.\n\nBOUNDS WIDTHxHEIGHT: %d x %d\n\nYou'll want to scooch one of the target_nulls: %d units.\n", width, height, delta);
	}

	//It's a square. Everything is ready to go...
	mapguiBound[0] = idVec2(topleftPos.x, topleftPos.y);
	mapguiBound[1] = idVec2(bottomrightPos.x, bottomrightPos.y);
}

idVec2 idMeta::GetMapguiNormalizedPositionViaEnt(idEntity* ent)
{
	if (ent == nullptr)
		return vec2_zero;

	idVec3 entityPosition = ent->GetPhysics()->GetOrigin();
	idVec2 infomapOffset = ent->infomapPositionOffset;
	if (infomapOffset != vec2_zero)
	{
		idVec3 forward, right;
		ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, NULL);
		entityPosition += (forward * infomapOffset.x) + (right * infomapOffset.y);
	}

	return GetMapguiNormalizedPosition(entityPosition);
}

idVec2 idMeta::GetMapguiNormalizedPosition(idVec3 entityPosition)
{
	return idVec2((entityPosition.x - mapguiBound[0].x) / (mapguiBound[1].x - mapguiBound[0].x), (entityPosition.y - mapguiBound[0].y) / (mapguiBound[1].y - mapguiBound[0].y));
}

//This is for total amount of enemies at game start. This is NOT updated as the game progresses.
int idMeta::GetTotalEnemyCount()
{
    return totalEnemies;
}

void idMeta::EquipEnemyLoadout()
{
	//Get the list of items that we want to distribute amongst all the baddies.
	idStr rawloadout = spawnArgs.GetString("def_enemyloadout", "");
	idStrList itemList = rawloadout.Split(',', true);
	if ( itemList.Num() <= 0 )
		return; //There are no items. Exit here.
	
	//Randomize the item list.
	itemList.Shuffle();

	
	int itemPerPersonCount = 0;

	//Equip the characters with these items.
	int itemIdx = 0;
	bool equipping = true;
	while (equipping)
	{
		//Iterate through every character. Give them an item.
		for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
		{
			//Skip player. Skip boss.
			if (!entity->spawnArgs.GetBool("randomdrops", "0"))
				continue;

			entity->spawnArgs.Set(idStr::Format("def_dropDeathItem%d", itemIdx + 2), itemList[itemIdx].c_str());
			itemIdx++;

			if (itemIdx >= itemList.Num())
			{
				equipping = false;
				break;
			}
		}

		if (itemIdx <= 0)
		{
			//There are NO enemies in the level. Exit.
			equipping = false;
			break;
		}

		itemPerPersonCount++;
		if (itemPerPersonCount >= MAX_ITEMS_PER_PERSON)
		{
			equipping = false;
			break;
		}
	}

}

void idMeta::SetSpaceNode(int navnode)
{
	//We are setting where the ship currently is here. 
	currentNavnode = navnode;

	//At the same time, we are going to determine where the ship is going next.	 We're just picking them randomly.
	if (navnode == NAVNODE_0)
	{
		nextNavnode = gameLocal.random.RandomInt(2) ? NAVNODE_1a : NAVNODE_1b;
	}
	else if (navnode == NAVNODE_1a || navnode == NAVNODE_1b)
	{
		nextNavnode = gameLocal.random.RandomInt(2) ? NAVNODE_2a : NAVNODE_2b;
	}
	else if (navnode == NAVNODE_2a || navnode == NAVNODE_2b)
	{
		nextNavnode = gameLocal.random.RandomInt(2) ? NAVNODE_3a : NAVNODE_3b;
	}
	else if (navnode == NAVNODE_3a || navnode == NAVNODE_3b)
	{
		nextNavnode = NAVNODE_PIRATEBASE;
	}

	if (developer.GetInteger() >= 1)
		common->Printf("NODE: just arrived at: %d  NEXT NODE: %d\n", currentNavnode, nextNavnode);

	//Update the info on all the guis. TODO: we can optimize this by making a list of all the infomaps, instead of iterating through every entity in the map.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (gameLocal.entities[i]->IsType(idInfoMap::Type))
		{
			static_cast<idInfoMap*>(gameLocal.entities[i])->SetCurrentNode(currentNavnode, nextNavnode);
		}
	}
}



void idMeta::Think(void)
{
	UpdateBeacon();

	if (combatMetastate == COMBATSTATE_SEARCH)
	{
		//BC hack to remove search timer logic.

		//gameLocal.GetLocalPlayer()->SetHudParmInt("searchtime", (int)(((1000 + searchTimer) - gameLocal.time) / 1000.0f));
		//
		//if (gameLocal.time > searchTimer)
		//{
		//	//Search state has expired.
		//	GotoCombatIdleState();
		//}
	}
	else if (combatMetastate == COMBATSTATE_COMBAT)
	{
		//12/6/2023 - combat state no longer times out. Stays on unless the player does an action to end it.

		//
		////Display gui of the combat timer.
		//float lerp = (gameLocal.time - combatstateStartTime) / (float)combatstateDurationTime;
		//lerp = 1.0f - idMath::ClampFloat(0, 1, lerp);
		//gameLocal.GetLocalPlayer()->hud->SetStateFloat("combatmeter", lerp);
		//
		//if (gameLocal.time >= combatstateStartTime + combatstateDurationTime)
		//{
		//	//do a failsafe hard limit to how long combat state can be.
		//	GotoCombatSearchState();
		//}
		//
	}


	//update the HUD element for combat,searching,clear.
	if (lastCombatMetastate != combatMetastate)
	{
		//gameLocal.GetLocalPlayer()->SetHudNamedEvent("combatstateFlash");

		if (combatMetastate == COMBATSTATE_IDLE)
		{
			gameLocal.GetLocalPlayer()->hud->SetStateInt("combatstate", 0);
			gameLocal.GetLocalPlayer()->SetHudNamedEvent("combatstateIdle");
		}
		else if (combatMetastate == COMBATSTATE_COMBAT)
		{
			gameLocal.GetLocalPlayer()->hud->SetStateInt("combatstate", 2);
		}
		else if (combatMetastate == COMBATSTATE_SEARCH)
		{
			gameLocal.GetLocalPlayer()->hud->SetStateInt("combatstate", 1);
		}

		if (lastCombatMetastate == COMBATSTATE_COMBAT)
		{
			Event_StopSpeakers(SND_CHANNEL_BODY); //Stop the combat alarm on the speakers.
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_combatalarm_end", SND_CHANNEL_VOICE2);
		}

		lastCombatMetastate = combatMetastate;
	}

	if (combatMetastate == COMBATSTATE_COMBAT)
	{
		//display combat timer on hud.
		float remainingCombatTime = combatstateDurationTime - ((gameLocal.time) - combatstateStartTime);

		idStr timeStr;
		if (remainingCombatTime >= combatstateDurationTime)
		{
			timeStr = "";
		}
		else
		{
			timeStr = gameLocal.ParseTimeMS_SecAndDecisec(remainingCombatTime);
		}

		gameLocal.GetLocalPlayer()->hud->SetStateString("combattimer", timeStr);



		//SWORDFISH TIMER
		float swordtimerLerp = (swordfishTimer - gameLocal.time) / (float)SWORDFISH_INTERVALTIME;
		swordtimerLerp = idMath::ClampFloat(0, 1, swordtimerLerp);
		gameLocal.GetLocalPlayer()->hud->SetStateFloat("swordtimer", 1.0f - swordtimerLerp);		

		if (gameLocal.time > swordfishTimer)
		{
			//swordfishtimer has maxed out.

			int securitybotsInRoom = GetSecurityBotsInRoom(gameLocal.GetLocalPlayer());
			if (securitybotsInRoom < SWORDFISH_SECURITY_MAXPERROOM && securitybotsInRoom >= 0) //clamp how much swordfish we have in a room.
			{
				//try to deploy swordfish in player's current room.
				if (SpawnSecurityBotInRoom(gameLocal.GetLocalPlayer()))
				{
					//print message that acknowledges swordfish has spawned.
					gameLocal.AddEventLog("#str_def_gameplay_alert_summonswordfish", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), true, EL_ALERT);

					gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("exclamationAlarm");
				}
			}

			//reset swordfishtimer.
			swordfishTimer = gameLocal.time + SWORDFISH_INTERVALTIME;
		}
	}
		

	UpdateInterestPoints();

	UpdateIdletasks();
	
	UpdateRepairManager();
	
	UpdateVentpurge();

	UpdateEscalationmeter();

	UpdateDynamicSky();

	UpdateAllClearSequence();


	//UpdateBombStartCheck();

	
	ReinforcementEndgameCheck();

	UpdatePlayerSpaceTimerStat();


	//if (gameLocal.world->spawnArgs.GetBool("meta_endcheck", "1") && enemydestroyedCheckState == ENEMYDESTROYEDSTATECHECK_WAITDELAY && gameLocal.time >= enemydestroyedCheckTimer && metaState == METASTATE_ACTIVE)
	//{
	//	enemydestroyedCheckState = ENEMYDESTROYEDSTATECHECK_NONE;
	//
	//	int enemiesRemaining = 0;
	//	int skullsRemaining = 0;
	//
	//	enemiesRemaining = GetEnemiesRemaining(false);
	//
	//	if (enemiesRemaining > 0)
	//		return; //Someone is still alive....... exit here
	//
	//	//Now we count the active skullsavers.
	//	for (idEntity* skullsaverEnt = gameLocal.skullsaverEntities.Next(); skullsaverEnt != NULL; skullsaverEnt = skullsaverEnt->skullsaverNode.Next())
	//	{
	//		if (!skullsaverEnt)
	//			continue;
	//
	//		if (skullsaverEnt->IsType(idSkullsaver::Type))
	//		{
	//			skullsRemaining++;
	//		}
	//	}
	//
	//	if ((enemiesRemaining <= 0 && skullsRemaining <= 0) || (enemiesRemaining <= 0 && skullsRemaining <= 1 && abs(gameLocal.time - lastKilltime) <= KILLCAM_THRESHOLDTIME))
	//	{
	//		if (metaState == METASTATE_ACTIVE)
	//		{
	//			if (SetupKillcam() && abs(gameLocal.time - lastKilltime) <= KILLCAM_THRESHOLDTIME) //If we find a good killcam spot AND the kill happened within the last 300 ms...
	//			{
	//				//Do the killcam sequence.
	//				metaState = METASTATE_KILLCAM;
	//				killcamTimer = gameLocal.slow.time + KILLCAM_DURATION;
	//				gameLocal.SetSlowmo(true);
	//
	//				gameLocal.GetLocalPlayer()->Hide(); //Just hide the player model.
	//			}
	//			else
	//			{
	//				//So, we couldn't find a valid spot for the killcam. That's ok. Just skip to the postgame.
	//				StartPostGame();
	//			}				
	//		}
	//	}
	//	//else
	//	//{
	//	//	common->Printf("Meta: enemies remaining: %d\n", enemiesRemaining);
	//	//}
	//
	//	//TODO: make the lifeboat auto leave???? Since the level is done...
	//}

	if (metaState == METASTATE_KILLCAM)
	{
		if (gameLocal.slow.time >= killcamTimer)
		{
			metaState = METASTATE_KILLCAM_WAITFORSLOWMO_END;
			gameLocal.SetSlowmo(false);			
		}		
	}
	else if (metaState == METASTATE_KILLCAM_WAITFORSLOWMO_END)
	{		
		if (gameLocal.slowmoState == SLOWMO_STATE_OFF)
		{
			((idCameraView *)killcamCamera)->SetActive(false);
			StartPostGame();
		}
	}

	if (walkietalkieLureTask.IsValid())
	{
		//Check if we still need to render the beam.
		if (walkietalkieLureTask.GetEntity()->IsType(idIdleTask::Type))
		{
			if (static_cast<idIdleTask *>(walkietalkieLureTask.GetEntity())->assignedActor.IsValid())
			{
				idVec3 actorPos = static_cast<idIdleTask *>(walkietalkieLureTask.GetEntity())->assignedActor.GetEntity()->GetPhysics()->GetOrigin();
				beamLure->SetOrigin(actorPos);
				beamRectangle->SetOrigin(actorPos + idVec3(0, 0, 40));
			}
		}
	}
	else if (!beamLure->IsHidden())
	{
		beamLure->Hide();
		beamRectangle->Hide();
	}

	
	
	UpdateDoorframeGUIs();
	
	if (reinforcementPhase == REINF_SPAWNDELAY)
	{
		if (gameLocal.time > reinforcementPhaseTimer)
		{
			reinforcementPhase = REINF_PIRATES_ENROUTE;
			SpawnPirateShip();
		}
	}

	if (signalkitDepletionCheckActive)
	{
		DoSignalkitDepletionCheck();
	}


	idGlassPiece::LimitActiveGlassPieces();

	//END THINK
}

void idMeta::ReinforcementEndgameCheck()
{
	if (!gameLocal.world->spawnArgs.GetBool("meta_endcheck", "1") || gameLocal.time < enemydestroyedCheckTimer || metaState != METASTATE_ACTIVE || reinforcementPhase != REINF_PIRATES_ALL_SPAWNED)
		return;

		
	int enemiesRemaining = 0;
	int skullsRemaining = 0;

	enemiesRemaining = GetEnemiesRemaining(false);

	if (enemiesRemaining > 0)
		return; //Someone is still alive....... exit here

	//Now we count the active skullsavers.
	for (idEntity* skullsaverEnt = gameLocal.skullsaverEntities.Next(); skullsaverEnt != NULL; skullsaverEnt = skullsaverEnt->skullsaverNode.Next())
	{
		if (!skullsaverEnt)
			continue;

		if (skullsaverEnt->IsType(idSkullsaver::Type))
		{
			skullsRemaining++;
		}
	}

	if ((enemiesRemaining <= 0 && skullsRemaining <= 0) || (enemiesRemaining <= 0 && skullsRemaining <= 1 && abs(gameLocal.time - lastKilltime) <= KILLCAM_THRESHOLDTIME))
	{
		if (metaState == METASTATE_ACTIVE)
		{
			if (SetupKillcam() && abs(gameLocal.time - lastKilltime) <= KILLCAM_THRESHOLDTIME) //If we find a good killcam spot AND the kill happened within the last 300 ms...
			{
				//Do the killcam sequence.
				metaState = METASTATE_KILLCAM;
				killcamTimer = gameLocal.slow.time + KILLCAM_DURATION;
				gameLocal.SetSlowmo(true);

				gameLocal.GetLocalPlayer()->Hide(); //Just hide the player model.
			}
			else
			{
				//So, we couldn't find a valid spot for the killcam. That's ok. Just skip to the postgame.
				StartPostGame();
			}
		}
	}	
}

bool idMeta::GetReinforcementsActive()
{
	return (reinforcementPhase != REINF_NONE);
}

void idMeta::UpdateDoorframeGUIs()
{
	if (!ftlGUIsAreOn)
		return;

	if (!GetFTLDrive.IsValid())
		return;

	int rawTimervalue = static_cast<idFTL *>(GetFTLDrive.GetEntity())->GetPublicTimer();
	if (rawTimervalue <= 0)
	{
		//FTL is currently ACTIVE. Countdown is done, ftl is currently on right now.
		if (doorframeInfoEnt.IsValid())
		{
			doorframeInfoEnt.GetEntity()->Event_SetGuiParm("ftltimer", "");
		}
		SetAirlockSignGuiValue("0");
		SetAirlockFTLDoorsignGui("0");
	}
	else
	{
		//COUNTDOWN is currently on.
		int seconds = rawTimervalue / 1000.0f;
		const char *textValue = idStr::Format("%d", seconds + 1);

		if (doorframeInfoEnt.IsValid())
		{
			doorframeInfoEnt.GetEntity()->Event_SetGuiParm("ftltimer", textValue);
		}

		SetAirlockSignGuiValue(textValue);
		SetAirlockFTLDoorsignGui(textValue);
	}
}

//Handle the doorframe ftl GUIs and the airlock ftl GUIs.
void idMeta::SetAll_FTL_Signage(int ftlsign_value)
{
	if (!doorframeInfoEnt.IsValid())
		return;

	if (ftlsign_value == FTLSIGN_COUNTDOWN)
	{
		//show the countdown timer.

		//Turn on the doorframe gui.
		//doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StartCountdown");
		ftlGUIsAreOn = true;

		//Show the airlock GUIs.
		for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
		{
			if (!airlockEnt)
				continue;

			if (!airlockEnt->IsType(idAirlock::Type))
				continue;

			static_cast<idAirlock *>(airlockEnt)->SetWarningSign(true);
			static_cast<idAirlock *>(airlockEnt)->SetFTLDoorGuiNamedEvent("StartCountdown");
		}
	}
	else if (ftlsign_value == FTLSIGN_OFF)
	{
		//FTL has been forcibly shut down.

		//doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StopFTL");
		ftlGUIsAreOn = false;

		for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
		{
			if (!airlockEnt)
				continue;

			if (!airlockEnt->IsType(idAirlock::Type))
				continue;

			//static_cast<idAirlock *>(airlockEnt)->SetWarningSign(false);
			static_cast<idAirlock *>(airlockEnt)->SetFTLDoorGuiNamedEvent("StopFTL");
		}
	}
	else if (ftlsign_value == FTLSIGN_ACTIVE)
	{
		//doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StartFTL");

		//Hide the airlock GUIs.
		for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
		{
			if (!airlockEnt)
				continue;

			if (!airlockEnt->IsType(idAirlock::Type))
				continue;

			static_cast<idAirlock *>(airlockEnt)->SetWarningSign(false);
			static_cast<idAirlock *>(airlockEnt)->SetFTLDoorGuiNamedEvent("StartFTL");
		}
	}
}


//void idMeta::UpdateDoorframeGUIs()
//{
//
//	//update the FTL ui stuff. Signage across ship, signs that pop out, etc.
//	bool ftlActive = false;
//	if (GetFTLDrive.IsValid())
//	{
//		ftlActive = static_cast<idFTL *>(GetFTLDrive.GetEntity())->IsJumpActive(false, true);
//	}
//
//	if (ftlActive)
//	{
//		//ftl is ON. Or, is counting down.
//
//		//Set timer.
//		int rawTimervalue = static_cast<idFTL *>(GetFTLDrive.GetEntity())->GetPublicTimer();
//		if (rawTimervalue <= 0)
//		{
//			//FTL is currently ACTIVE. Countdown is done, ftl is currently on right now.
//			if (doorframeInfoEnt.IsValid())
//			{
//				doorframeInfoEnt.GetEntity()->Event_SetGuiParm("ftltimer", "000");
//			}
//			SetAirlockSignGuiValue("000");
//		}
//		else
//		{
//			//COUNTDOWN is currently on.
//			int seconds = rawTimervalue / 1000.0f;
//			const char *textValue = idStr::Format("%03d", seconds + 1);
//
//			if (doorframeInfoEnt.IsValid())
//			{
//				doorframeInfoEnt.GetEntity()->Event_SetGuiParm("ftltimer", textValue);
//			}
//
//			SetAirlockSignGuiValue(textValue);
//			lastFTLActiveState = FTL_COUNTDOWN;
//		}
//
//		//see if we need to deactivate the countdown UI.
//		if (lastFTLActiveState == FTL_COUNTDOWN && rawTimervalue <= 0)
//		{
//			if (doorframeInfoEnt.IsValid())
//			{
//				doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StopFTL");
//			}
//
//			lastFTLActiveState = FTL_ACTIVE;
//
//			//Hide the airlock warning signs.
//			
//		}
//
//		lastFTLActiveState = ftlActive;
//	}
//	else
//	{
//		//ftl is OFF.
//
//		//see if we need to start the ftl countdown ui stuff.
//		if (lastFTLActiveState != FTL_COUNTDOWN)
//		{
//			if (doorframeInfoEnt.IsValid())
//			{
//				doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StartFTL");
//			}
//
//			lastFTLActiveState = FTL_COUNTDOWN;
//
//			//Activate airlock warning signage
//			for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
//			{
//				if (!airlockEnt)
//					continue;
//
//				if (!airlockEnt->IsType(idAirlock::Type))
//					continue;
//
//				static_cast<idAirlock *>(airlockEnt)->SetWarningSign(true);
//			}
//		}
//	}
//}

void idMeta::SetAirlockSignGuiValue(const char *text)
{
	for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
	{
		if (!airlockEnt)
			continue;

		if (!airlockEnt->IsType(idAirlock::Type))
			continue;

		static_cast<idAirlock *>(airlockEnt)->SetWarningSignText(text);
		return; //all of these guis are synced, so we only need to do it to one airlock.
	}
}

void idMeta::SetAirlockFTLDoorsignGui(const char *text)
{
	for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
	{
		if (!airlockEnt)
			continue;

		if (!airlockEnt->IsType(idAirlock::Type))
			continue;

		static_cast<idAirlock *>(airlockEnt)->SetFTLDoorGuiDisplaytime(text);
	}
}

//This gets called ONCE when the player completes the final objective.
//The player has VICTORIOUSLY COMPLETED a mission.
void idMeta::StartPostGame()
{
	if (metaState == METASTATE_POSTGAME)
		return;

	//Achievement for completing level.

	if (common->g_SteamUtilities && common->g_SteamUtilities->IsSteamInitialized() && !gameLocal.GetLocalPlayer()->isInVignette)
	{		
		idStr mapname;
		mapname = gameLocal.GetMapName();
		mapname.StripPath();
		mapname.StripFileExtension();
		mapname.ToLower();

		//Grab map def of current map.
		const idDecl* mapDecl = declManager->FindType(DECL_MAPDEF, mapname, false);
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(mapDecl);
		if (mapDef)
		{
			//found the map def.
			idStr levelinternalname = mapDef->dict.GetString("internalname");
			if (levelinternalname.Length() > 0)
			{
				idStr leveldoneAchievementName = idStr::Format("ach_leveldone_%s", levelinternalname.c_str());
				common->g_SteamUtilities->SetAchievement(leveldoneAchievementName);
			}
		}		
	}


	metaState = METASTATE_POSTGAME;
	
	if (gameLocal.GetLocalPlayer()->isInVignette)
	{
		//Vignette.
		gameLocal.RunMapScript("_OnVictory");
	}
	else
	{
		// SW 24th Feb 2025: Adding an extra script function here for the spectate postgame,
		// so we can turn off certain annoying level behaviours
		gameLocal.RunMapScript("_OnPostGame");

		gameLocal.GetLocalPlayer()->SetSpectateVictoryFanfare();
		gameLocal.GetLocalPlayer()->ServerSpectate(true);

		//Pause the world.
		gameLocal.spectatePause = true;
	}
}

bool idMeta::SetupKillcam()
{
	//Find the right place to put the camera.

	if (!killcamTarget.IsValid())
		return false;

	if (!killcamTarget.GetEntity())
		return false;

	//Ok, try to find a good location.
	idVec3 cameraPos = FindKillcamPosition();

	if (cameraPos == vec3_zero)
		return false; //Failed to find a valid camera position.

	idDict args;
	args.Clear();
	args.Set("classname", "func_cameraview");
	args.SetInt("fov", KILLCAM_FOV);
	args.Set("attachedView", killcamTarget.GetEntity()->GetName());
	args.SetVector("origin", cameraPos);
	gameLocal.SpawnEntityDef(args, &killcamCamera);

	if (!killcamCamera)
		return false;

	((idCameraView *)killcamCamera)->SetActive(true);
	return true;
}

idVec3 idMeta::FindKillcamPosition()
{
	idVec3 candidatePos;
	idVec3 targetCenter = killcamTarget.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter();

	//First: try something from direction of player.	
	idVec3 dirToPlayer = gameLocal.GetLocalPlayer()->firstPersonViewOrigin - targetCenter;
	dirToPlayer.Normalize();
	candidatePos = targetCenter + dirToPlayer * KILLCAM_DISTANCE;
	candidatePos.z = targetCenter.z + KILLCAM_HEIGHTOFFSET;
	if (ValidateKillcamPosition(candidatePos))
		return candidatePos; //position is good! Return it.
	
	//Ok, that failed. That's ok. Try finding a position perpendicular to that.
	idAngles crossAngle;
	crossAngle = dirToPlayer.ToAngles();
	crossAngle.pitch = 0;
	crossAngle.yaw += 90;
	candidatePos = targetCenter + crossAngle.ToForward() * KILLCAM_DISTANCE;
	candidatePos.z = targetCenter.z + KILLCAM_HEIGHTOFFSET;
	if (ValidateKillcamPosition(candidatePos))
		return candidatePos; //position is good! Return it.	
	
	//Ok, that failed. That's ok. Try the angle 180 degrees from there.
	crossAngle.yaw += 180;
	candidatePos = targetCenter + crossAngle.ToForward() * KILLCAM_DISTANCE;
	candidatePos.z = targetCenter.z + KILLCAM_HEIGHTOFFSET;
	if (ValidateKillcamPosition(candidatePos))
		return candidatePos; //position is good! Return it.

	//Ok, that failed. That's ok. Try a position raised up high.
	candidatePos = targetCenter + dirToPlayer * KILLCAM_DISTANCE / 3;
	candidatePos.z = targetCenter.z + 80;
	if (ValidateKillcamPosition(candidatePos))
		return candidatePos; //position is good! Return it.

	return vec3_zero; //fail.
}

bool idMeta::ValidateKillcamPosition(idVec3 position)
{
	//do a traceline.
	idVec3 targetCenter = killcamTarget.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter();
	trace_t tr;
	
	gameLocal.clip.TraceBounds(tr, targetCenter, position, idBounds(idVec3(-4, -4, -4), idVec3(4, 4, 4)), MASK_SOLID, NULL);

	return (tr.fraction >= 1.0f);
}

void idMeta::UpdateDynamicSky(void)
{
	if (skyController.IsValid())
	{
		int skyOverride = g_skyOverride.GetInteger();
		if (skyOverride >= 0)
		{
			static_cast<idSkyController*>(skyController.GetEntity())->SetCurrentSky(skyOverride);
		}
		else
		{
			// Put whatever normal sky changing logic you want here.
		}
	}
}


void idMeta::UpdateEscalationmeter()
{
	if (escalationmeterTimer > gameLocal.time)
		return;

	escalationmeterTimer = gameLocal.time + ESCALATIONMETER_UPDATETIME;

	if (!IsFTLActive())
	{
		//Check if anyone can see player.
		int amountWhoCanSeePlayer = 0;
		for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
		{
			if (!entity->IsActive() || entity->IsHidden() || entity == this)
			{
				continue;
			}

			//Do not target friendlies.
			if (entity->IsType(idActor::Type))
			{
				if (static_cast<idActor*>(entity)->team != TEAM_ENEMY)
				{
					continue; //if you're not a bad guy, then we're SKIPPING you.
				}

				//ok, we have a bad guy now.
				if (entity->IsType(idAI::Type) && entity->health > 0)
				{
					if (static_cast<idAI*>(entity)->lastFovCheck)
					{
						amountWhoCanSeePlayer++;
					}
				}
			}
		}


		if (amountWhoCanSeePlayer > 0 && (combatMetastate == COMBATSTATE_COMBAT || combatMetastate == COMBATSTATE_SEARCH))
		{
			//Increase the meter.
			escalationmeterAmount = min(escalationmeterAmount + ESCALATIONMETER_INCREASEDELTA, ESCALATIONMETER_MAX);


			//if (escalationmeterAmount >= ESCALATIONMETER_MAX)
			//{
			//	//Activate the FTL jump.
			//	if (GetFTLDrive.IsValid())
			//	{
			//		static_cast<idFTL *>(GetFTLDrive.GetEntity())->StartCountdown();
			//	}
			//}

		}
		else if (combatMetastate == COMBATSTATE_IDLE && escalationmeterAmount > 0)
		{
			//De-escalate.
			escalationmeterAmount = max(escalationmeterAmount - ESCALATIONMETER_DECREASEDELTA, 0);
		}


		//Update gui meter.
		float lerpAmount = escalationmeterAmount / (float)ESCALATIONMETER_MAX;
		gameLocal.GetLocalPlayer()->hud->SetStateFloat("escalationmeter", lerpAmount);
	}
}

bool idMeta::IsFTLActive()
{
	if (!GetFTLDrive.IsValid())
		return false;

	return static_cast<idFTL *>(GetFTLDrive.GetEntity())->IsJumpActive(true);
}

void idMeta::Event_GetLastSwordfishTime()
{
	idThread::ReturnFloat(lastSwordfishTime);
}

void idMeta::Event_SetSwordfishTime(float value)
{
	lastSwordfishTime = value;
}

void idMeta::UpdateBeacon()
{
	//Make the beacon orbit around 0,0,0
	idVec3 centerPos = vec3_zero;
	centerPos.x += idMath::Cos(beaconAngle) * BEACON_DISTANCE;
	centerPos.y += idMath::Sin(beaconAngle) * BEACON_DISTANCE;
	//centerPos.z += idMath::Sin(gameLocal.time * BEACON_VERTICALSPEED) * BEACON_VERTICALMAX; //Make it wiggle up and down
	centerPos.z += gameLocal.GetLocalPlayer()->firstPersonViewOrigin.z; //Remove the Z axis, just have it be at the same eye level as the player.
	beaconPosition = centerPos;

	beaconAngle += BEACON_SPEED;

	//gameRenderWorld->DebugLine(colorWhite, centerPos + idVec3(0, 0, 2048), centerPos, 100, false);
}

void idMeta::Event_GetBeaconPosition()
{
	idThread::ReturnVector(beaconPosition);
}


//This function determines whether the LKP model should be visible or invisible. This function is not polled every frame.
//This function is called when:
//- An ai has GAINED line of sight to enemy. 
//- An ai has LOST line of sight to enemy.
//- An ai dies.

//hasGainedLOS = has an actor gained LOS on enemy within the last frame.
void idMeta::UpdateMetaLKP(bool hasGainedLOS)
{
	int amountWhoCanSeePlayer = 0;
	int amountInCombatState = 0;
	int amountSearching = 0;
    int amountSkullsavers = 0;
	idVec3 viewerPos = vec3_zero;

	//Iterate through all ai and see if all or none have line of sight. We piggyback onto the auto aim system.
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden() || entity == this)
		{
			continue;
		}
		
		if (entity->team != TEAM_ENEMY || entity->health <= 0) //only care about querying bad guys. and alive.
			continue;
		
		if (entity->IsType(idActor::Type))
		{
			//ok, we have a bad guy now.
			if (entity->IsType(idAI::Type))
			{
				if (static_cast<idAI*>(entity)->lastFovCheck)
				{
					amountWhoCanSeePlayer++;
					viewerPos = entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
				}

				if (static_cast<idAI*>(entity)->combatState > 0 || static_cast<idAI*>(entity)->aiState == AISTATE_COMBAT)
				{
					amountInCombatState++;
				}

				if (static_cast<idAI*>(entity)->aiState == AISTATE_SEARCHING || static_cast<idAI*>(entity)->aiState == AISTATE_OVERWATCH)
				{
					amountSearching++;
				}

			}
		}

		if (entity->IsType(idTurret::Type))
		{
			//Query the turret...			
			if (static_cast<idTurret *>(entity)->IsInCombat()) //If turret is engaged in combat, then we consider it in combat state + can see enemy.
			{
				amountWhoCanSeePlayer++;
			}

			if (static_cast<idTurret *>(entity)->IsOn())
			{
				amountInCombatState++;
			}
		}
		
		if (entity->IsType(idSecurityCamera::Type))
		{
			if (static_cast<idSecurityCamera *>(entity)->IsAlerted())
			{
				amountInCombatState++;
			}
		}
	}

	if (amountWhoCanSeePlayer > 0)
	{
		//At least one AI can see player. Make the LKP invisible.
		if (!lkpEnt->IsHidden())
		{
			SetLKPVisible(false);
		}
	}
	else if (amountWhoCanSeePlayer <= 0)
	{
		if (lkpEnt->IsHidden() && amountInCombatState > 0)
		{
			//Zero AI have LOS to the player. Make the LKP turn on.
			SetLKPVisible(true);
		}
	}


    //Get skull count.
    for (idEntity* skullsaverEnt = gameLocal.skullsaverEntities.Next(); skullsaverEnt != NULL; skullsaverEnt = skullsaverEnt->skullsaverNode.Next())
    {
        if (!skullsaverEnt)
            continue;

        if (skullsaverEnt->IsType(idSkullsaver::Type))
        {
            amountSkullsavers++;
        }
    }



	if (combatMetastate == COMBATSTATE_SEARCH && amountSearching <= 0 && amountWhoCanSeePlayer <= 0 && amountSearching <= 0 && amountInCombatState <= 0)
	{
		//If in search state and no one is searching: then go to idle state.
		GotoCombatIdleState();
	}
	//else if (combatMetastate == COMBATSTATE_COMBAT && amountSearching > 0 && amountInCombatState <= 0)
	//{
	//	//If in combat state, and people are searching, and no one in combat state: then go to search state.
	//	GotoCombatSearchState();
	//}
	else if (amountInCombatState <= 0 && amountSearching <= 0 && amountWhoCanSeePlayer <= 0 && amountSearching <= 0)
	{
		//common->Printf("Go to combat idle state     %d in combat, %d searching, %d can see player\n", amountInCombatState, amountSearching, amountWhoCanSeePlayer);
		//If no one in combat state and no one in search state: then go to idle state.
		GotoCombatIdleState();
	}

	if (hasGainedLOS)
	{
		SetAllClearInterrupt();
	}

	//If we're in the middle of a vent purge or airlock purge, we check if the AI has seen the player OUTSIDE the vent or airlock.
	//If enemy does see player outside of vent or airlock, we cancel the "purge done, we think the player is dead all-clear".
	if (hasGainedLOS)
	{
		//check if player is in vent or airlock.
		enemyThinksPlayerIsInVentOrAirlock = IsEntInVentOrAirlock(gameLocal.GetLocalPlayer());
	}
}


//This handles when to make screen flash white ("You've been sighted by enemy")
void idMeta::UpdateSighted()
{
	int amountInCombatState = 0;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden() || entity == this)
		{
			continue;
		}

		//Do not target friendlies.
		if (entity->IsType(idActor::Type))
		{
			if (static_cast<idActor*>(entity)->team != 1)
			{
				continue; //if you're not a bad guy, then we're SKIPPING you.
			}

			//ok, we have a bad guy now.
			if (entity->IsType(idAI::Type) && entity->health > 0)
			{
				if (static_cast<idAI*>(entity)->combatState > 0)
				{
					amountInCombatState++;
				}
			}
		}
	}

	if (amountInCombatState <= 1)
	{
		gameLocal.GetLocalPlayer()->playerView.DoSightedFlash();

		this->StartSound("snd_sighted", SND_CHANNEL_ANY, 0, false, NULL);
	}
}



//This function is how AI "alert" each other to the enemy's location. It grabs all the AI within the player's PVS and sends them toward the enemy LKP.
//This also shifts the worldstate into Combat state.
void idMeta::AlertAIFriends(idEntity *caller)
{
	enemiesKnowOfNina = true;

	#define MAX_INVESTIGATORQUOTA 2
	int investigatorQuota = 0;

	//hijack the autoaim system. Iterate over all baddies. See which ones are within player's PVS.
 	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (entity->IsHidden())
			continue;

		if (caller == NULL)
		{
			if (entity->IsType(idAI::Type))
			{
				if (static_cast<idActor*>(entity)->team == TEAM_ENEMY)
				{
					//set ai to alerted state, but without an LKP or anything like that
					static_cast<idAI*>(entity)->SetAlertedState(false);
				}
			}
			continue;
		}

		if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0 || entity->entityNumber == caller->entityNumber || entity->team != TEAM_ENEMY)		//sanity check
			continue;		

		if (!entity->IsType(idAI::Type))
			continue;

		if (!static_cast<idAI *>(entity)->CanAcceptStimulus())
			continue;

		if (!gameLocal.InPlayerPVS(entity) || !gameLocal.InPlayerConnectedArea(entity) || investigatorQuota >= MAX_INVESTIGATORQUOTA)
		{
			//only care about actors within player's PVS.
			//For actors who are NOT in player's PVS, we still want them to "look alert". They'll wander around the ship.
			static_cast<idAI*>(entity)->SetAlertedState(false);
			continue;
		}
		
		if (static_cast<idActor*>(entity)->team != TEAM_ENEMY || static_cast<idAI*>(entity)->lastFovCheck)
			continue; //if you're NOT a bad guy, or if you already have LOS to enemy, then we're SKIPPING you.

		//ok, we have a bad guy now. Tell them to switch to alerted state.
		static_cast<idAI*>(entity)->SetAlertedState(true);
		investigatorQuota++;
	}

	if (combatMetastate != COMBATSTATE_COMBAT)
	{
		//make the speakers do the combat alarm sound.
		// SW 17th March 2025: Combat alarm shouldn't be on the voice channel, switching to body
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_combatalarm", SND_CHANNEL_BODY);
	}

	//Reset the global combat state.
	combatMetastate = COMBATSTATE_COMBAT;
	combatstateStartTime = gameLocal.time;
	combatstateDurationTime = COMBATSTATE_FAILSAFE_EXITTIME;
	
	//Make all the doorframes show the info gui.
	if (doorframeInfoEnt.IsValid())
	{
		doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StartCombat");
	}


	//Start FTL jump, if necessary.
	IncreaseEscalationLevel();


	//Wake up all the turrets.
	ActivateAllTurrets(true);

	//If I see player in vent, then do vent purge.
	//if (caller->IsType(idAI::Type))
	//{
	//	if (static_cast<idAI*>(caller)->lastEnemySeen.IsValid())
	//	{
	//		if (static_cast<idAI*>(caller)->lastEnemySeen.GetEntity() == gameLocal.GetLocalPlayer())
	//		{
	//			idVec3 playerPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
	//			if (gameLocal.GetPositionIsInsideConfinedspace(playerPosition))
	//			{prn
	//				common->Printf("startventpurge: AI saw player inside vent.\n");
	//				gameLocal.GetLocalPlayer()->confinedStealthActive = false;
	//				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartVentPurge();
	//			}
	//		}
	//	}
	//}

	//Lock down all health stations.
	for (int i = 0; i < gameLocal.healthstationEntities.Num(); i++)
	{
		if (!gameLocal.healthstationEntities[i])
			continue;

		static_cast<idHealthstation *>(gameLocal.healthstationEntities[i])->SetCombatLockdown(true);
	}

	//Stop the radio checkin.
	if (radioCheckinEnt != NULL)
	{
		//If radio checkin is happening, make it stop.
		radioCheckinEnt->StopCheckin();
		radioCheckinEnt->ResetCooldown();
	}

	swordfishTimer = gameLocal.time + SWORDFISH_INTERVALTIME;	
}

//GOTO IDLE STATE.
void idMeta::GotoCombatIdleState()
{
	//Unlock health stations when idle state happens.
	for (int i = 0; i < gameLocal.healthstationEntities.Num(); i++)
	{
		if (!gameLocal.healthstationEntities[i])
			continue;

		static_cast<idHealthstation *>(gameLocal.healthstationEntities[i])->SetCombatLockdown(false);
	}	

	combatMetastate = COMBATSTATE_IDLE;
	searchTimer = 0;

	if (doorframeInfoEnt.IsValid())
	{
		doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StartIdle");
	}

	if (gameLocal.GetLocalPlayer())
	{
		gameLocal.GetLocalPlayer()->confinedStealthActive = true; //If player lost confinedStealth ability, then we re-enable it here.
	}

	//Deactivate turrets.
	ActivateAllTurrets(false);

	//Kick every AI back into idle state.
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden() || entity->team != TEAM_ENEMY)
			continue;

		if (!entity->IsType(idAI::Type))
			continue;

		if (!static_cast<idAI *>(entity)->CanAcceptStimulus())
			continue;

		//Go to AI idle state.
		static_cast<idAI *>(entity)->GotoState(AISTATE_IDLE);
	}

	UnlockAirlockLockdown();
}

//ENTER SEARCH STATE.
void idMeta::GotoCombatSearchState()
{
	combatMetastate = COMBATSTATE_SEARCH;

	//Update doorframe gui info panels.
	//if (doorframeInfoEnt.IsValid())
	//{
	//	doorframeInfoEnt.GetEntity()->Event_GuiNamedEvent(1, "StartSearch");
	//}

	//Unlock health stations when search state happens.
	for (int i = 0; i < gameLocal.healthstationEntities.Num(); i++)
	{
		if (!gameLocal.healthstationEntities[i])
			continue;
	
		static_cast<idHealthstation *>(gameLocal.healthstationEntities[i])->SetCombatLockdown(false);
	}

	//Un-deploy all turrets when search state happens.
	ActivateAllTurrets(false);


	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0 || entity->team != TEAM_ENEMY)
			continue;

		if (!entity->IsType(idAI::Type))
			continue;
		
		if (!static_cast<idAI *>(entity)->CanAcceptStimulus())
			continue;

		//if I'm already in alert state, then skip me.
		if (static_cast<idAI *>(entity)->aiState != AISTATE_IDLE)
			continue;

		static_cast<idAI*>(entity)->SetAlertedState(false); //make them alert, but not investigate LKP
	}
}

void idMeta::ActivateAllTurrets(bool value)
{
	for (idEntity* entity = gameLocal.turretEntities.Next(); entity != NULL; entity = entity->turretNode.Next())
	{
		if (!entity)
			continue;

		if (entity->IsType(idTurret::Type))
		{
			if (entity->team == TEAM_FRIENDLY) //don't interact with turrets that the player has hacked/hijacked.
				continue;

			static_cast<idTurret *>(entity)->Event_activate(value ?  1 : 0);
		}
	}
}



// -------------------------------- WALL SPEAKERS ----------------------------------------

void idMeta::Event_ActivateSpeakers(const char *soundshaderName, const s_channelType soundchannel)
{
	//Get all the speaker entities in the map. And make them play a sound shader.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (gameLocal.entities[i]->entityNumber == this->entityNumber || !gameLocal.entities[i]->IsType(idWallspeaker::Type))
			continue;

		static_cast<idWallspeaker *>(gameLocal.entities[i])->ActivateSpeaker(soundshaderName, soundchannel);
	}
}

void idMeta::Event_StopSpeakers(const s_channelType soundchannel)
{
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (!gameLocal.entities[i]->IsType(idWallspeaker::Type))
			continue;

		static_cast<idWallspeaker *>(gameLocal.entities[i])->StopSound(soundchannel);
	}
}





// ------------------------------ PIPES -------------------------------------

bool idMeta::SetPipeStatus(int pipeIndex, bool value)
{
	if (pipeIndex < 0 || pipeIndex >= MAX_PIPESTATUSES)
	{
		gameLocal.Error("SetPipeStatus: pipeIndex out of range (pipeIndex = %d)", pipeIndex);
		return false;
	}

	pipeStatuses[pipeIndex] = value;


	//Now do a check to see if we should shut down the FTL.
	if (!pipeStatuses[PIPE_HYPERFUEL] && !pipeStatuses[PIPE_ELECTRICITY] && !pipeStatuses[PIPE_NAVCOMPUTER])
	{
		static_cast<idFTL *>(GetFTLDrive.GetEntity())->SetPipePauseState(); //Tell FTL to enter pause state.

		//make all PRIMED sabotagelevers become ARMED, and all others be colored red.
		for (int i = 0; i < gameLocal.num_entities; i++)
		{
			if (!gameLocal.entities[i])
				continue;
		
			//if (gameLocal.entities[i]->entityNumber == this->entityNumber ) //skip self.
			//	continue;
		
			if (!gameLocal.entities[i]->IsType(idSabotageLever::Type))
				continue;
		
			static_cast<idSabotageLever *>(gameLocal.entities[i])->SetSabotagedArmed();
		}

		return true;//total shutdown.
	}

	return false;//single button.
}

// ------------------------------ LAST KNOWN POSITION -------------------------------------

void idMeta::GetLKPPosition()
{
	idThread::ReturnVector(lkpEnt->GetPhysics()->GetOrigin());
}

void idMeta::SetLKPVisible(bool visible)
{
	if (visible)
	{
		if (lkpEnt->IsHidden())
		{
			idEntityFx::StartFx("fx/smoke_ring01", &lkpEnt->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
			lkpEnt->StartSound("snd_lkp_spawn", SND_CHANNEL_ANY, 0, false, NULL);
		}
		
		//lkpEnt->SetOrigin(lkpEnt->GetPhysics()->GetOrigin());
		lkpEnt->Show();
		lkpEnt_Eye->Show();
		lkpLocbox->Show(); //BC 3-24-2025: locbox.

		// On shutdown, it's possible the local player is null. Just early-out in that case.
		if ( !gameLocal.GetLocalPlayer() )
		{
			return;
		}
		
		//Check if LKP is inside a vent space, and if so, start the vent purge.
		if (gameLocal.GetPositionIsInsideConfinedspace(lkpEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, 1)))
		{
			gameLocal.AddEventLog("#str_def_gameplay_vent_seeninside", lkpEnt->GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);

			gameLocal.GetLocalPlayer()->confinedStealthActive = false;
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartVentPurge(nullptr); //We don't really track who saw the player, so just use generic voiceprint A...
		}

		//Check if LKP is inside an airlock. If so, start airlock purge.
		for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
		{
			if (!airlockEnt)
				continue;

			if (!airlockEnt->IsType(idAirlock::Type))
				continue;

			//See if airlock is already purged.
			if (static_cast<idAirlock *>(airlockEnt)->IsAirlockLockdownActive())
				continue;

			//See if LKP *and* player are inside the airlock.
			idVec3 playerPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
			if (airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(lkpEnt->GetPhysics()->GetOrigin() + idVec3(0,0,1))
				&& airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(playerPosition))
			{
				//LKP is inside airlock. Activate the lockdown.
				gameLocal.AddEventLog("#str_def_gameplay_airlock_seenenter", playerPosition);

				static_cast<idAirlock *>(airlockEnt)->DoAirlockLockdown(true);
				continue;
			}
		}

	}
	else
	{
		if (!lkpEnt->IsHidden())
		{
			idEntityFx::StartFx("fx/smoke_ring01", &lkpEnt->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		}

		lkpEnt->Hide();
		lkpEnt_Eye->Hide();
		lkpLocbox->Hide(); //BC 3-24-2025: locbox.
	}
}

void idMeta::SetLKPPositionByEntity(idEntity *enemyEnt)
{
	idVec3 reachablePos;
	if (enemyEnt->GetPhysics()->HasGroundContacts())
	{
		//Has ground contact. Is reachable. TODO: check if this point is reachable by AAS???
		reachablePos = enemyEnt->GetPhysics()->GetContact(0).point;
	}
	else
	{
		//No ground contact. Drop to ground below.
		trace_t tr;
		gameLocal.clip.TracePoint(tr, enemyEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, -1), enemyEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, -1024), MASK_SOLID, this);
		reachablePos = tr.endpos;

		//TODO: make sure AI can reach this point.
	}

	SetLKPReachablePosition(reachablePos);
	SetLKPPosition(enemyEnt->GetPhysics()->GetOrigin());
}

void idMeta::SetLKPPosition(idVec3 newPosition)
{
	//Round it to int. So that we can make a more reliable comparison to its previous location.
	newPosition.x = (int)newPosition.x;
	newPosition.y = (int)newPosition.y;
	newPosition.z = (int)newPosition.z;

	//If already at same place, don't make it appear again.
	if (newPosition.x == lkpEnt->GetPhysics()->GetOrigin().x
		&& newPosition.y == lkpEnt->GetPhysics()->GetOrigin().y
		&& newPosition.z == lkpEnt->GetPhysics()->GetOrigin().z
		&& !lkpEnt->IsHidden())
	{
		return;
	}

	//gameRenderWorld->DebugArrow(colorMagenta, newPosition + idVec3(0, 0, 128), newPosition, 8, 10000);
	//common->Printf("new lkp at %f %f %f", lkpEnt->GetPhysics()->GetOrigin().x, lkpEnt->GetPhysics()->GetOrigin().y, lkpEnt->GetPhysics()->GetOrigin().z);
	
	lkpEnt->SetOrigin(newPosition);
}

//We have the concept of LastKnownPosition (LKP) and LastKnownPositionReachable. The "reachable" variant is what the AI uses as a fallback in case they can't traverse to the LKP, i.e. if the LKP is floating in the air. The "reachable" variant stores position of the ground contacts.
void idMeta::SetLKPReachablePosition(const idVec3 &newPosition)
{
	lkpReachablePosition = newPosition;
	//gameRenderWorld->DebugArrow(colorGreen, newPosition + idVec3(0, 0, 128), newPosition, 4, 60000);
}

void idMeta::Event_GetLKPReachable()
{
	idThread::ReturnVector(lkpReachablePosition);
}

//This gets called when a baddie is killed.
void idMeta::IncreaseEscalationLevel()
{
	//if (currentNavnode >= NAVNODE_PIRATEBASE)
	//{
	//	return; //at pirate base. no more escalation.
	//}
	//
	//escalationLevel++;
	//
	//gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("escalationIncrease");
	//gameLocal.GetLocalPlayer()->hud->SetStateInt("escalationlevel", escalationLevel);
	//
	//if (escalationLevel >= ESCALATION_MAX)
	//{
	//	//Escalation has maxed out. Do the hyperspace jump.
	//	//common->Printf("baboo hyperspace\n");
	//
    //    if (GetFTLDrive.IsValid())
    //    {
	//		if (!static_cast<idFTL *>(GetFTLDrive.GetEntity())->IsJumpActive(true))
	//		{
	//			static_cast<idFTL *>(GetFTLDrive.GetEntity())->StartCountdown();
	//		}
    //    }
	//
	//	if (bossEnt.IsValid())
	//	{
	//		static_cast<idBossMonster *>(bossEnt.GetEntity())->StartBossTeleportSequence();
	//	}		
	//}
	//
	//gameLocal.GetLocalPlayer()->hud->SetStateString("escalation_surplus", (escalationLevel <= ESCALATION_MAX) ? "" : idStr::Format("+%d", escalationLevel - ESCALATION_MAX));


	int enemiesRemaining = GetEnemiesRemaining(false); //get amount of alive enemies; do not include skullsavers

	if (enemiesRemaining <= 0)
		return;//don't do speaker stuff if every bad guy is dead.

	if (GetFTLDrive.IsValid())
	{
		if (!static_cast<idFTL *>(GetFTLDrive.GetEntity())->IsJumpActive(true))
		{
			static_cast<idFTL *>(GetFTLDrive.GetEntity())->StartCountdown();
		}
	}

	escalationLevel = ESCALATION_MAX;

	SetAllClearInterrupt(); //cancel the walkie talkie all-clear sequence
}

void idMeta::SetAllInfostationsMode(int mode)
{
	int stationToIgnore = -1;
	if (gameLocal.GetLocalPlayer()->GetFocusedGUI() != NULL)
	{
		//If the player is currently looking at an infostation, we don't want the station to suddenly flip to the FTL screen while they're using it.
		//Ignore the station if the player is currently using it.
		stationToIgnore = gameLocal.GetLocalPlayer()->GetFocusedGUI()->entityNumber;
	}

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idInfoStation::Type))
		{
			if (ent->entityNumber == stationToIgnore)
				continue;

			static_cast<idInfoStation *>(ent)->SetToMode(mode); //Switch to FTL screen.
		}
	}
}

void idMeta::InfostationsLockdisplayUpdate()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idInfoStation::Type))
		{
			static_cast<idInfoStation *>(ent)->DoDelayedDoorlockDisplayUpdate();
		}
	}

	gameLocal.GetLocalPlayer()->DoDoorlockDelayedUpdate();
}

bool idMeta::IsEscalationMaxed()
{
	if (escalationLevel >= ESCALATION_MAX)
		return true;

	return false;
}

void idMeta::Event_GetEscalationLevel()
{
	idThread::ReturnInt(escalationLevel);
}

//This gets called AFTER each FTL jump.
void idMeta::ResetEscalationLevel()
{
	escalationLevel = max(escalationLevel - ESCALATION_MAX, 0);
	gameLocal.GetLocalPlayer()->hud->SetStateInt("escalationlevel", escalationLevel);

	//Also, update the next hyperspace jump nav nodes.
	SetSpaceNode(nextNavnode);

	//Reset escalationmeter.
	escalationmeterAmount = 0;
	
	gameLocal.GetLocalPlayer()->hud->SetStateString("escalation_surplus", (escalationLevel <= ESCALATION_MAX) ? idStr("") : idStr::Format("+%d", escalationLevel - ESCALATION_MAX ));

	if (skyController.IsValid())
	{
		static_cast<idSkyController *>(skyController.GetEntity())->NextSky(); //BC test, iterate to next skybox.
	}

	if (enemiesEliminatedInCurrentPhase > 0)
	{
		for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
		{
			if (!entity->IsActive() || entity->IsHidden() || entity->team != TEAM_ENEMY)
				continue;

			if (!entity->IsType(idActor::Type))
				continue;

			if (!entity->spawnArgs.GetBool("healthincreases", "0"))
				continue;

			//Append the health.
			static_cast<idActor *>(entity)->SetMaxHealthDelta(MAXHEALTH_FTL_INCREASE);
		}

		gameLocal.GetLocalPlayer()->SetHudNamedEvent("ftl_healthincrease");
		enemiesEliminatedInCurrentPhase = 0;
	}

	//Release airlock lockdown.
	UnlockAirlockLockdown();

	SetAllInfostationsMode(INFOS_MAPDISPLAY);

	//Seal up all the busted windows.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		//if (gameLocal.entities[i]->IsType(idSkullsaver::Type))
		//{
		//	static_cast<idSkullsaver *>(gameLocal.entities[i])->doRespawnJump();
		//}
		
		if (gameLocal.entities[i]->IsType(idWindowseal::Type))
		{
			static_cast<idWindowseal *>(gameLocal.entities[i])->PostFTLReset();
		}
	}

	//BC this respawns skullsavers when the FTL is done. Disable this for Skullsaver 3.0
	//RespawnSkullsavers();
}

void idMeta::UnlockAirlockLockdown()
{
	for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
	{
		if (!airlockEnt)
			continue;

		if (!airlockEnt->IsType(idAirlock::Type))
			continue;

		if (!static_cast<idAirlock *>(airlockEnt)->IsAirlockLockdownActive())
			continue;

		static_cast<idAirlock *>(airlockEnt)->DoAirlockLockdown(false);
	}
}


void idMeta::RespawnSkullsavers()
{
	//Respawn the skullsavers.
	//First, find the quantity of spawn points we need.
	idList<int> skullsaverEntityNums;
	for (idEntity* skullsaverEnt = gameLocal.skullsaverEntities.Next(); skullsaverEnt != NULL; skullsaverEnt = skullsaverEnt->skullsaverNode.Next())
	{
		if (!skullsaverEnt)
			continue;

		if (skullsaverEnt->IsType(idSkullsaver::Type))
		{
			if (!static_cast<idSkullsaver *>(skullsaverEnt)->bodyOwner.IsValid())
			{
				gameLocal.Warning("Skullsaver '%s' has no valid body owner.", skullsaverEnt->GetName());
				continue;
			}

			if (!static_cast<idSkullsaver *>(skullsaverEnt)->bodyOwner.GetEntity()->IsType(idAI::Type))
			{
				gameLocal.Warning("Skullsaver '%s' body owner is not idAI.", skullsaverEnt->GetName());
				continue;
			}


			skullsaverEntityNums.Append(skullsaverEnt->entityNumber);
		}
	}

	if (skullsaverEntityNums.Num() <= 0)
		return; //There are no skullsavers to respawn.

	gameLocal.DPrintf("Found %d skullsavers to respawn.", skullsaverEntityNums.Num());

	//Great, we now know how many spawnpoints we need.
	//Now we generate a list of the enemy spawnpoints in the level.

	spawnSpot_t		spot;
	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnspots;

	spot.ent = gameLocal.FindEntityUsingDef(NULL, "info_enemyspawnpoint");
	if (!spot.ent)
	{
		//Uh oh.... no enemy respawn point found. Oh well! Enemy just doesn't respawn.
		gameLocal.Warning("Skullsavers attempting to teleport respawn, but can't find a valid spawnpoint.");
		return;
	}

	while (spot.ent)
	{
		if (gameLocal.GetAirlessAtPoint(spot.ent->GetPhysics()->GetOrigin() + idVec3(0,0,1)))
		{
			//Spot has no oxygen. Skip it.......
			common->Warning("RespawnSkullsavers: is airless at '%s'\n", spot.ent->GetName());
			spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint"); //Continue to next spawn spot.
			continue;
		}

		//Do clearance check.
		idBounds bodyBounds = idBounds(idVec3(-12, -12, 0), idVec3(12, 12, 72)); //bounds for what will spawn at the spot.
		trace_t tr;
		idVec3 tracePos = spot.ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 2);
		gameLocal.clip.TraceBounds(tr, tracePos, tracePos, bodyBounds, MASK_SOLID, spot.ent);		
		if (tr.fraction < 1)
		{
			//Something is blocking the spot. Skip it........
			common->Warning("RespawnSkullsavers: something is blocking '%s'\n", spot.ent->GetName());
			spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint"); //Continue to next spawn spot.
			continue;
		}

		//Calculate distance and store it.
		float dist = (spot.ent->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthSqr();
		spot.dist = dist;
		spawnspots.Append(spot);

		spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint"); //Continue to next spawn spot.
	}

	//We now have an array of all the spawnspots. Sort them by distance.
	qsort((void *)spawnspots.Ptr(), spawnspots.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Farthest);

	if (spawnspots.Num() < skullsaverEntityNums.Num()) //Sanity check to make sure we have enough spawnpoints.
	{
		gameLocal.Warning("skullsavers don't have enough respawn points (skullsavers: %d, respawn point candidates: %d)\n", skullsaverEntityNums.Num(), spawnspots.Num());

		//TODO: have a fallback for this.......
	}

	skullsaverEntityNums.Shuffle(); //Randomize the order that the enemies respawn.

	//TODO: it would be nice if the spawnpoint order was randomized a bit so that the skullsaver is not 100% always the furthest one from the player. This isn't a huge deal
	//but solves the wonkiness of the player being able to control exactly where baddies will respawn.

	for (int i = 0; i < skullsaverEntityNums.Num(); i++)
	{
		if (!gameLocal.entities[skullsaverEntityNums[i]]->IsType(idSkullsaver::Type))
			continue;

		idVec3 respawnSpot = spawnspots[i].ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
		static_cast<idSkullsaver *>(gameLocal.entities[skullsaverEntityNums[i]])->doRespawnJump(respawnSpot);
	}
}

void idMeta::PirateBaseSpawnCheck()
{
	//TODO: make a boolean so that this doesn't get activated more than once.

	//Check whether we just arrived at the pirate base.
	if (currentNavnode < NAVNODE_PIRATEBASE)
	{
		return;
	}

	//we are at the pirate base. Spawn the boss.
	if (bossEnt.IsValid())
	{
		static_cast<idBossMonster *>(bossEnt.GetEntity())->StartBossTeleportSequence();
	}


	//TODO: spawn baddies... detect for victory condition.
}


//Called when an enemy is stored in lifeboat or escape pod. We add a delay so that we can more accurately count who is dead and alive; otherwise we might get a false positive if we call this right when an actor is destroyed.
void idMeta::OnEnemyStoredLifeboat()
{
	if (enemydestroyedCheckState != ENEMYDESTROYEDSTATECHECK_REINFORCEMENTPHASE)
		return;

	enemydestroyedCheckState = ENEMYDESTROYEDSTATECHECK_WAITDELAY;
	enemydestroyedCheckTimer = gameLocal.time + 300;	
	
	enemiesEliminatedInCurrentPhase++;
}

//Called when an enemy is killed. This is for killcam logic.
void idMeta::OnEnemyDestroyed(idEntity *ent)
{
	if (reinforcementPhase != REINF_PIRATES_ALL_SPAWNED)
		return;

	enemydestroyedCheckState = ENEMYDESTROYEDSTATECHECK_WAITDELAY;
	enemydestroyedCheckTimer = gameLocal.time + 10;

	if (ent != NULL)
	{
		killcamTarget = ent;
		lastKilltime = gameLocal.time;
	}
}


bool idMeta::GenerateNodeLOS(idVec3 pointToObserve)
{
	//BC create array of nodes that can see a specific point.
	//The way this works is we iterate over all the searchnodes and
	//does tracelines. This is a very brute force approach.

	pvsHandle_t		pvs;
	int				localPvsArea;	

	if ((int)pointToObserve[0] == lastNodeSearchPos[0] && (int)pointToObserve[1] == lastNodeSearchPos[1] && (int)pointToObserve[2] == lastNodeSearchPos[2])
		return true; //Already have this info. Don't re-generate the same data.

	lastNodeSearchPos = idVec3((int)pointToObserve[0], (int)pointToObserve[1], (int)pointToObserve[2]); //Keep a record of the last point, so that we're not doing unnecessary data re-generation of the same spot. We're converting floats to int so that we can compare the number.

	localPvsArea = gameLocal.pvs.GetPVSArea(pointToObserve);
	pvs = gameLocal.pvs.SetupCurrentPVS(localPvsArea);

	nodePosArraySize = 0;

	//Iterate over all search nodes.
	for (idEntity* entity = gameLocal.searchnodeEntities.Next(); entity != NULL; entity = entity->aiSearchNodes.Next())
	{
		trace_t tr;
		idVec3 eyePos;
		idVec3 adjustedObservepoint;
		float lengthToPoint;		

		if (nodePosArraySize >= MAX_LOS_NODES)
			break;

		eyePos = entity->GetPhysics()->GetOrigin() + idVec3(0, 0, MONSTER_EYEHEIGHT);

		if (!gameLocal.pvs.InCurrentPVS(pvs, eyePos))
		{
			continue;
		}		

		adjustedObservepoint = pointToObserve + idVec3(0, 0, gameLocal.GetLocalPlayer()->EyeHeight());		
		gameLocal.clip.TracePoint(tr, eyePos, adjustedObservepoint, MASK_SOLID, NULL);

		lengthToPoint = (adjustedObservepoint - tr.endpos).LengthFast();

		if (lengthToPoint < SEARCHNODE_LOS_DISTANCE_THRESHOLD || tr.fraction >= 1.0f)
		{
			//Searchnode is valid. Add to array.
			nodePositions[nodePosArraySize] = entity->GetPhysics()->GetOrigin();
			nodePosArraySize++;

			//gameRenderWorld->DebugArrow(colorCyan, eyePos, adjustedObservepoint, 4, 10000);
		}
		else
		{
			//Check the spots around the interestpoint.

			trace_t bodyTr;			
			idBounds bodyBounds;

			//total number of points: 24
			idVec3 offsets[] = {
				idVec3(0,32,0),
				idVec3(0,-32,0),
				idVec3(32,0,0),
				idVec3(-32,0,0),

				idVec3(32,32,0),
				idVec3(32,-32,0),
				idVec3(-32,-32,0),
				idVec3(-32,32,0),

				idVec3(0,64,0),
				idVec3(0,-64,0),
				idVec3(64,0,0),
				idVec3(-64,0,0),

				idVec3(64,64,0),
				idVec3(64,-64,0),
				idVec3(-64,-64,0),
				idVec3(-64,64,0),

				idVec3(0,96,0),
				idVec3(0,-96,0),
				idVec3(96,0,0),
				idVec3(-96,0,0),

				idVec3(96,96,0),
				idVec3(96,-96,0),
				idVec3(-96,-96,0),
				idVec3(-96,96,0)
			};

			bodyBounds = idBounds(idVec3(-24, -24, 0), idVec3(24, 24, 74)); //bounding box for human actor.
			

			for (int i = 0; i < 24; i++)
			{
				idVec3 candidatePos;
				trace_t floorTr;

				candidatePos = entity->GetPhysics()->GetOrigin() + offsets[i];

				gameLocal.clip.TracePoint(floorTr, entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), entity->GetPhysics()->GetOrigin() + idVec3(0, 0, -2), MASK_MONSTERSOLID, NULL); //Check if there's a floor to stand on.
				if (floorTr.fraction >= 1)
					continue;

				gameLocal.clip.TraceBounds(bodyTr, candidatePos + idVec3(0,0,0.1f), candidatePos + idVec3(0, 0, 0.1f), bodyBounds, MASK_SOLID, NULL); //Check if this space can fit the human bounding box.
				
				//gameRenderWorld->DebugBounds((bodyTr.fraction >= 1.0f) ? colorCyan : colorRed, bodyBounds, candidatePos, 10000);

				if (bodyTr.fraction >= 1)
				{
					//Ok, this position is unobstructed. And has a floor. Great. Do the LOS check now.
					eyePos = candidatePos + idVec3(0, 0, MONSTER_EYEHEIGHT);
					gameLocal.clip.TracePoint(tr, eyePos, adjustedObservepoint, MASK_SOLID, NULL);

					//gameRenderWorld->DebugArrow(colorGreen, eyePos, tr.endpos, 4, 10000);

					lengthToPoint = (adjustedObservepoint - tr.endpos).LengthFast();

					if (lengthToPoint < SEARCHNODE_LOS_DISTANCE_THRESHOLD || tr.fraction >= 1.0f)
					{
						//Valid point. Add to array.
						nodePositions[nodePosArraySize] = candidatePos;
						nodePosArraySize++;
						i = 999;
					}
				}
			}
		}
	}

	gameLocal.pvs.FreeCurrentPVS(pvs);
	
	if (nodePosArraySize > 0)
	{
		return true; //Success. Found at least one valid spot.
	}

	gameLocal.Warning("GenerateNodeLOS() couldn't find a suitable LOS node to: %.0f %.0f %.0f", pointToObserve.x, pointToObserve.y, pointToObserve.z);
	return false; //Fail. Couldn't find a spot that has LOS to pointToObserve.
}


void idMeta::Event_GetSearchTimer()
{
	idThread::ReturnFloat(GetSearchTimer());
}

int idMeta::GetSearchTimer()
{
	return gameLocal.time + 10000; //BC hack to make searchtimer go forever

	//if (combatMetastate == COMBATSTATE_COMBAT)
	//{
	//	//If combattimer is currently in play, then searchtimer is always maxed out.
	//	searchTimer = gameLocal.time + (g_searchtime.GetInteger() * 1000);
	//}
	//
	//return searchTimer;
}

//Someone in the world has started searching.
void idMeta::SomeoneStartedSearching()
{
	int amountInCombatState = 0;

	//Someone has started a search. Reset search timer to maximum.
	searchTimer = gameLocal.time + (g_searchtime.GetInteger() * 1000);	

	//Check if anyone is in combat state. If NO ONE is in combat state, then we officially shift the global combat state to SEARCH.
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden() || entity == this)
		{
			continue;
		}

		//Do not target friendlies.
		if (entity->IsType(idAI::Type))
		{
			if (static_cast<idActor*>(entity)->team != TEAM_ENEMY)
			{
				continue; //if you're not a bad guy, then we're SKIPPING you.
			}

			//ok, we have a bad guy now.
			if (entity->health > 0 && static_cast<idAI*>(entity)->combatState > 0)
			{
				amountInCombatState++;
			}
		}
	}

	
	//If no one is in combat state, then we downgrade to search state. This gets activated when during combat state,
	//the baddies investigate the LKP and find nothing there.
	//if (amountInCombatState <= 0 && combatMetastate == COMBATSTATE_IDLE)
	//{
	//	GotoCombatSearchState();
	//}
}



idVec3 idMeta::GetLKPReachablePosition()
{
	return lkpReachablePosition;
}

// SW: Clear all interestpoints from the world. Intended for vignette/scripting use
void idMeta::ClearInterestPoints()
{
	for (idEntity* entity = gameLocal.interestEntities.Next(); entity != NULL; entity = entity->interestNode.Next())
	{
		if (!entity)
			continue;

		if (entity->IsHidden())
			continue;

		entity->PostEventMS(&EV_Remove, 0);
	}
}

idEntity * idMeta::SpawnInterestPoint(idEntity *ownerEnt, idVec3 position, const char *interestName)
{
	if (interestName[0] == '\0') //if def is empty, then skip.
		return NULL;

	idEntity				*interestpoint;
	const idDeclEntityDef	*interestDef;
	interestDef = gameLocal.FindEntityDef(interestName);

	// SW 5th May 2025: Avoid spawning interestpoints outside the world. But do some little checks to see if it's borderline (e.g. coplanar to a wall)
	if (gameRenderWorld->PointInArea(position) == -1)
	{
		#define	INTEREST_OFFSETCOUNT 6
		bool inWorld = false;
		idVec3 testPos;
		idVec3 offsets[INTEREST_OFFSETCOUNT] = {
			idVec3(0, 0, 1),
			idVec3(0, 0, -1),
			idVec3(0, 1, 0),
			idVec3(0, -1, 0),
			idVec3(1, 0, 0),
			idVec3(-1, 0, 0)
		};

		for (int i = 0; i < INTEREST_OFFSETCOUNT; i++)
		{
			testPos = position + offsets[i];
			if (gameRenderWorld->PointInArea(testPos) != -1)
			{
				inWorld = true;
				break;
			}
		}
		
		if (inWorld)
		{
			// Modify our position to be inside the world
			position = testPos;
		}
		else
		{
			// Return NULL to indicate that the interestpoint didn't spawn
			return NULL;
		}
	}
		

	// SW: See if there are any interestpoints of the same type inside our 'duplicate radius' (if so, we refrain from spawning).
	// This helps avoid spamming unnecessary interestpoints for more chaotic stimuli (things exploding, shattering, bouncing all over the place, etc)
	int duplicateRadius = interestDef->dict.GetInt("duplicateRadius", 0);
	if (duplicateRadius > 0)
	{
		for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next()) {
			if (
				ent->IsType(idInterestPoint::Type) &&
				(ent->GetPhysics()->GetOrigin() - position).LengthFast() < duplicateRadius &&
				!idStr(interestDef->GetName()).Icmp(ent->GetEntityDefName())
				)
			{
				// An interestpoint of the exact same definition already exists within this radius. Do not spawn.
				return NULL;
			}
		}
	}
	

	if (!interestDef)
	{
		gameLocal.Error("SpawnInterestPoint() failed to find entity def: '%s'\n", interestName);
		return NULL;
	}

	if (gameLocal.GetAirlessAtPoint(position) || gameLocal.IsBaffled(position))
	{
		const char *interestType = interestDef->dict.GetString("type");
		if (!idStr::Icmp(interestType, "noise"))
		{
			//a NOISE happened in ZERO G. Ignore it; there's no audio in space.
			return NULL;
		}
	}


	gameLocal.SpawnEntityDef(interestDef->dict, &interestpoint, false);

	if (!interestpoint)
	{
		gameLocal.Error("SpawnInterestPoint() found def but failed to spawn entity: '%s'\n", interestName);
		return NULL;
	}

	interestpoint->SetOrigin(position);

	if (g_showInterestPoints.GetBool())
	{
		#define INTERESTPOINT_DEBUGDRAWTIME 10000
		gameRenderWorld->DebugBounds(colorGreen, idBounds(idVec3(-2, -2, -2), idVec3(2, 2, 2)), position, INTERESTPOINT_DEBUGDRAWTIME);

		//randomly offset the text to avoid text overlapping one another
		idVec3 randomOffset = idVec3(gameLocal.random.RandomInt(-8, 8), gameLocal.random.RandomInt(-8, 8), gameLocal.random.RandomInt(4, 32));
		idVec3 textPos = position + randomOffset;
		gameRenderWorld->DebugLine(colorWhite, position, textPos + idVec3(0, 0, -2), INTERESTPOINT_DEBUGDRAWTIME);
		gameRenderWorld->DrawText(interestpoint->GetName(), textPos, .1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, INTERESTPOINT_DEBUGDRAWTIME);
	}

	if (interestpoint->spawnArgs.GetInt("bound", "0") > 0)
	{
		//Bind to the entity.
		if (ownerEnt)
		{
			interestpoint->Bind(ownerEnt, false);
		}
	}

	interestpoint->GetPhysics()->GetClipModel()->SetOwner(ownerEnt);

	//particle fx.
	const char *particlename = interestpoint->spawnArgs.GetString("smoke_model");
	if (particlename[0] != '\0')
	{
		gameLocal.DoParticle(particlename, position);
	}

	if (interestpoint->IsType(idInterestPoint::Type) && ownerEnt != NULL)
	{
		static_cast<idInterestPoint *>(interestpoint)->SetOwnerDisplayName(ownerEnt->displayName);
		static_cast<idInterestPoint*>(interestpoint)->interestOwner = ownerEnt;
	}

	return interestpoint;
}

//This gets called every frame.
void idMeta::UpdateInterestPoints()
{
	//Iterate through every interest point.
	for (idEntity* entity = gameLocal.interestEntities.Next(); entity != NULL; entity = entity->interestNode.Next())
	{
		idInterestPoint *interestpoint;		

		if (!entity)
			continue;

		if (entity->IsHidden())
			continue;

		interestpoint = static_cast<idInterestPoint *>(entity);

		if (!interestpoint)
			continue;				

		if (interestpoint->isClaimed)
		{
			if (!interestpoint->claimant.IsValid() ||
				!interestpoint->claimant.GetEntity()->GetLastInterest().IsValid() ||
				interestpoint->claimant.GetEntity()->GetLastInterest().GetEntity() != interestpoint)
			{
				idStr interestpointName = interestpoint->GetName();
				if (interestpoint->displayName[0] != '\0')
				{
					interestpointName = interestpoint->displayName;
				}

				idStr claimantName = "?";
				if (interestpoint->claimant.IsValid())
				{
					claimantName = interestpoint->claimant.GetEntity()->displayName;
				}

				gameLocal.Warning("interestpoint ('%s') claimant ('%s') is invalid/distracted.", interestpointName.c_str(), claimantName.c_str());
			}
			continue;
		}
		

		// Remove interestpoints that have expired
		if (gameLocal.time > interestpoint->GetExpirationTime() && interestpoint->GetExpirationTime() > 0)
		{
			interestpoint->PostEventMS(&EV_Remove, 0);
			continue;
		}

		// SW: Remove interestpoints that have ceased to be remarkable *and* have no active observers/investigators 
		// (e.g. stuff that's been discreetly repaired)
		if (interestpoint->cleanupWhenUnobserved && !interestpoint->isClaimed && interestpoint->observers.Num() == 0)
		{
			interestpoint->PostEventMS(&EV_Remove, 0);
			continue;
		}

		if (gameLocal.time > interestpoint->sensoryTimer)
		{
			interestpoint->sensoryTimer = gameLocal.time + SENSORY_UPDATETIME;
			idStaticList<spawnSpot_t, MAX_GENTITIES> candidates;
			idVec3 closestWalkablePosition = vec3_zero;

			//We have an interestnode that is ready to be sensory-updated.
			
			//Check it against every ai in the level.
			for (idEntity* aiEnt = gameLocal.aimAssistEntities.Next(); aiEnt != NULL; aiEnt = aiEnt->aimAssistNode.Next())
			{
				if (!aiEnt || aiEnt->IsHidden() || aiEnt->health <= 0 || !aiEnt->IsType(idAI::Type) || aiEnt->team != TEAM_ENEMY)
					continue;
				
				idAI* ai = static_cast<idAI*>(aiEnt);

				if (ai->InterestPointCheck(interestpoint))
				{
					//Is candidate. Add to candidate list.
					spawnSpot_t newcandidate;
					newcandidate.ent = aiEnt;

					// SW: Choose how we measure 'distance' from the interestpoint. 
					// Some of these methods could end up kind of expensive, since we're potentially polling a large number of AIs all at once,
					// but some of them can have kinda stupid results.
					// For now, I'm wrapping this in a cvar so we can easily compare approaches.
					if (g_meta_candidateDistanceMetric.GetInteger() == 2)
					{
						// Full pathfinding approach: query the AI for how they'd pathfind to the interestpoint
						// We're concerned this might be too expensive.
						// (though to be fair, AI outside of the interestpoint's radius have already been discarded at this point)
						idVec3 interestVec = interestpoint->GetPhysics()->GetOrigin();
						
						// This is the 'closest' walkable position to the interestpoint that the AI could find.
						// It's an expensive-as-hell check, but all the AIs *should* arrive at the same conclusion,
						// so we only need to make this call once.
						if (closestWalkablePosition == vec3_zero)
							closestWalkablePosition = ai->FindValidPosition(interestVec);
						 
						// TravelDistance() is also an expensive call :(
						newcandidate.dist = ai->TravelDistance(aiEnt->GetPhysics()->GetOrigin(), closestWalkablePosition) + (closestWalkablePosition - interestVec).LengthFast();
					}
					else if (g_meta_candidateDistanceMetric.GetInteger() == 1)
					{
						// Weighted approach: We get a straight-line distance, but punish AIs that:
						// - are high above/below the interestpoint
						// - don't have line-of-sight (if it's a sound)
						// - are in a separate area
						// This should hopefully be not much more expensive than the basic approach, and may alleviate some of the sillier edge-cases
						//bool lineOfSight = true;
						//bool sameArea = false;
						trace_t trace;
						idVec3 aiVec = ai->GetEyePosition();
						idVec3 interestVec = interestpoint->GetPhysics()->GetOrigin();
						idVec3 dist = aiVec - interestVec;

						// Favour AIs whose distance is primarily on the X-Y plane
						dist.z *= 3.0f; 

						// Favour AIs who have line-of-sight to sound interestpoints (assume all AIs have line-of-sight for visual interestpoints)
						if (interestpoint->interesttype == IPTYPE_NOISE && gameLocal.clip.TracePoint(trace, aiVec, interestVec, MASK_OPAQUE, aiEnt))
							dist *= 1.5f;

						// Favour AIs who are in the same location as the interestpoint
						int aiArea = gameRenderWorld->PointInArea(aiVec);
						int interestArea = gameRenderWorld->PointInArea(interestVec);
						if (aiArea != -1 && interestArea != -1 && aiArea != interestArea)
							dist *= 1.5f;
						
						newcandidate.dist = dist.LengthFast();
					}
					else
					{
						// Basic approach: Just get the straight-line distance, regardless of orientation or intervening obstacles
						// (This can produce some funky results, but maybe players won't notice?)
						newcandidate.dist = (ai->GetEyePosition() - interestpoint->GetPhysics()->GetOrigin()).LengthFast();
					}
					candidates.Append(newcandidate);
				}
			}

			if (candidates.Num() <= 0)
				// There are zero candidates currently available for this interestpoint. 
				// We will try again the next time we update -- until we get one, or until the interestpoint expires.
				continue; 

			//Sort by distance.
			if (candidates.Num() >= 1)
			{
				qsort((void*)candidates.Ptr(), candidates.Num(), sizeof(spawnSpot_t), (int(*)(const void*, const void*))gameLocal.sortSpawnPoints_Nearest);
			}

			
			//Ok, we have candidates. Do some quasi group coordination by assigning roles to each person.
			interestpoint->SetClaimed(true, static_cast<idAI*>(candidates[0].ent)); // Closest candidate investigates (our 'claimant')
			for (int i = 0; i < candidates.Num(); i++)
			{
				idAI* candidate = static_cast<idAI*>(candidates[i].ent);
				
				// Forget our previous interest point, if valid
				if (candidate->GetLastInterest().IsValid())
				{
					// The candidate may have been interested in the interestpoint as a claimant, or as an observer -- try to remove both types
					idInterestPoint* lastInterestPoint = static_cast<idInterestPoint*>(candidate->GetLastInterest().GetEntity());
					if (lastInterestPoint->HasObserver(candidate))
						lastInterestPoint->RemoveObserver(candidate);
					else if (lastInterestPoint->claimant.GetEntity() == candidate)
						lastInterestPoint->SetClaimed(false);
					else
						gameLocal.Warning("idMeta::UpdateInterestPoints: Candidate is forgetting about an interestpoint, but the interestpoint did not forget about the candidate.");

					// Clear it from the candidate's end
					candidate->ClearInterestPoint();
				}

				if (i > 0)
				{
					interestpoint->AddObserver(candidate);
				}

				// Closest candidate investigates. Everyone else (who is not presently occupied otherwise) provides overwatch.
				candidate->InterestPointReact(interestpoint, i <= 0 ? INTERESTROLE_INVESTIGATE : INTERESTROLE_OVERWATCH);
				if (!candidate->GetLastInterest().IsValid() || candidate->GetLastInterest().GetEntity() != interestpoint)
				{
					//gameRenderWorld->DebugArrow(colorRed, candidate->GetPhysics()->GetOrigin(), interestpoint->GetPhysics()->GetOrigin(), 4, 10000);

					//gameRenderWorld->DebugArrow(colorGreen, candidate->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), candidate->GetPhysics()->GetOrigin(), 4, 20000);
					//gameRenderWorld->DebugTextSimple((i <= 0) ? "INVESTIGATOR" : "OVERWATCH", candidate->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), 20000);
					//common->Printf("React person '%s'  interestpoint '%s'\n", candidate->GetName(), interestpoint->GetName());
					//gameRenderWorld->DebugTextSimple(idStr::Format("claimed: %d\n", interestpoint->isClaimed).c_str(), interestpoint->GetPhysics()->GetOrigin(), 20000);
					
					
					if (!candidate->GetLastInterest().IsValid())
					{
						gameLocal.Warning("idMeta::UpdateInterestPoints: '%s' failed to show interest in assigned interestpoint (GetLastInterest() is null)", candidate->GetName());
					}
					else
					{
						gameLocal.Warning("idMeta::UpdateInterestPoints: '%s' failed to show interest in assigned interestpoint (GetLastInterest() '%s' doesn't match new interestpoint '%s')",
							candidate->GetName(), candidate->GetLastInterest().GetEntity()->GetName(), interestpoint->GetName());
					}
				}
			}
		}
	}
}

// SW: Something has been repaired (e.g. a smashed light). It may have an interestpoint associated with it,
// in which case we need to clean it up.
void idMeta::InterestPointRepaired(idEntity* repairEnt)
{
	// First step: from the repair ent, we need to find the interestpoint that is owned by it.
	// If we can't find any, that's okay -- the broken entity may already have been fully investigated.
	idInterestPoint* interestPoint;
	for (idEntity* entity = gameLocal.interestEntities.Next(); entity != NULL; entity = entity->interestNode.Next())
	{
		if (!entity)
			continue;

		if (entity->IsHidden())
			continue;

		interestPoint = static_cast<idInterestPoint*>(entity);

		if (interestPoint && interestPoint->IsBoundTo(repairEnt))
		{
			// The logic here is: If nobody has noticed the broken thing yet, it ceases to be noticeable once it has been repaired,
			// (assuming repair bots are capable of magic like-new repairs on everything they fix).
			// However, if it is in the process of being investigated *when the repair happens*, 
			// guards should not magically lose interest mid-investigation.

			// Our solution is to set the 'cleanupWhenUnobserved' flag to true. If the interestPoint is currently claimed, or observed,
			// it will continue to behave as normal. If it is unobserved, it will be cleaned up on the next update.

			interestPoint->cleanupWhenUnobserved = true;
			break;
		}
	}
}

// SW: An interest point has been investigated.
// Go through our guards and ensure none of them have a lingering reference to it, then delete it.
void idMeta::InterestPointInvestigated(idEntity* entity)
{
	idInterestPoint* interestPoint;
	if (!entity->IsType(idInterestPoint::Type))
	{
		gameLocal.Warning("idMeta::InterestPointInvestigated: Was supplied something that is not an interest point!");
		return;
	}
	else
	{
		interestPoint = static_cast<idInterestPoint*>(entity);
	}

	idAI* ai = static_cast<idAI*>(interestPoint->claimant.GetEntity());
	if (ai != NULL)
	{
		ai->ClearInterestPoint();
	}

	for (int i = 0; i < interestPoint->observers.Num(); i++)
	{
		if (interestPoint->observers[i].IsValid()) {
			interestPoint->observers[i].GetEntity()->ClearInterestPoint();
		}
	}
	interestPoint->ClearObservers();
	interestPoint->SetClaimed(false);
	interestPoint->PostEventMS(&EV_Remove, 0);
}

// The investigator of this interest point was distracted by something else 
// (like another interest point, or perhaps something more pressing, like combat).
void idMeta::InterestPointDistracted(idEntity* interestPointEnt, idAI* distractedAI)
{
	idInterestPoint* interestPoint;

	if (interestPointEnt == NULL || !interestPointEnt->IsType(idInterestPoint::Type))
	{
		gameLocal.Warning("idMeta::InterestPointDistracted: Was supplied something that is not an interest point!");
		return;
	}
	else
	{
		interestPoint = static_cast<idInterestPoint*>(interestPointEnt);

		//BC Don't get distracted by the interestpoint the AI is already investigating.
		//if (interestPoint->claimant.IsValid())
		//{
		//	if (interestPoint->claimant.GetEntityNum() == distractedAI->entityNumber)
		//	{
		//		return;
		//	}
		//}		
	}

	// First possible case: the investigator is distracted, so we try to find someone to pick their target up
	if (distractedAI == interestPoint->claimant.GetEntity())
	{
		bool overwatchToInvestigate = false;
		for (int i = 0; i < interestPoint->observers.Num(); i++)
		{
			if (!interestPoint->observers[i].IsValid())
				continue;

			idAI* aiEnt = interestPoint->observers[i].GetEntity();
			if (!aiEnt || !aiEnt->IsType(idAI::Type))
				continue;

			// If there are any remaining AIs observing this, switch one of them to investigate instead.
			idAI* ai = static_cast<idAI*>(aiEnt);
			if (ai->GetLastInterest().IsValid() && ai->GetLastInterest().GetEntity() == interestPoint && ai != distractedAI)
			{
				ai->ClearInterestPoint();
				interestPoint->RemoveObserver(ai);
				interestPoint->SetClaimed(true, ai);
				ai->InterestPointReact(interestPoint, INTERESTROLE_INVESTIGATE);
				
				overwatchToInvestigate = true;
				break;
			}
		}

		// If nobody else picked up on this interestpoint
		if (!overwatchToInvestigate)
		{
			interestPoint->ClearObservers();
			interestPoint->SetClaimed(false); // Interestpoint can now be reassigned the next time we update.
		}
	}
	// Second possible case: an observer is distracted.
	else
	{
		interestPoint->RemoveObserver(distractedAI);
	}
	
	// Finally, sever the connection from the AI's end.
	distractedAI->ClearInterestPoint();
}

//Check and see what entities are requesting to be repaired.
void idMeta::UpdateRepairManager()
{
	if (repairmanagerTimer > gameLocal.time || !g_monsters.GetBool() || !g_repairenabled.GetBool())
		return;	

	repairmanagerTimer = gameLocal.time + REPAIRMANAGER_UPDATETIME;	

	//Check for anything that needs to be repaired. Iterate over all 'broken' things in the world.
	for (idEntity* entity = gameLocal.repairEntities.Next(); entity != NULL; entity = entity->repairNode.Next())
	{
		//idEntity * availableBot = NULL;

		if (!entity)
			continue;

		if (!entity->needsRepair || entity->repairrequestTimestamp + REPAIRMANAGER_REPAIRDELAY > gameLocal.time)
			continue;


		//Has it already been claimed?
		if (HasRepairEntityBeenClaimed(entity))
			continue;

		//This entity needs to be repaired.
		if (entity->spawnArgs.GetBool("repair_verify", "1"))
		{
			if (!entity->repairRequestVerified)
			{
                //Entity has not been 'observed' by an AI.

				//Check if an AI has LOS to the object.
				if (DoesEnemyHaveLOStoRepairable(entity))
				{
					entity->repairRequestVerified = true;

                    //add a delay before the bot gets called.
                    entity->repairrequestTimestamp = gameLocal.time + REPAIRMANAGER_REPAIR_VO_LINE_LENGTH - REPAIRMANAGER_REPAIRDELAY;
                    return;
				}

				if (!entity->repairRequestVerified)
				{
					continue; //Has not been verified/noticed by an AI. Skip for now...
				}
			}
		}

		//If another bot is ALREADY on their way to the repairable, then don't do anything. Basically, we trust that bot will eventually do the job.
		if (IsBotRepairingSomethingNearby(entity))
		{
			continue;
		}

		//First, try to find an existing repairbot that can repair this thing.
		if (SendAvailableRepairBot(entity))
		{
			//Successfully sent the bot.
			continue;
		}

		//See if max repairbots on the field has been reached.
		if (GetRepairBotCount() >= gameLocal.world->spawnArgs.GetInt("maxrepairbots", "4"))
		{
			continue;
		}		

		//Activate a repairbot spawner. And assign task to it.
		ActivateClosestRepairHatch(entity);
	}
}

int idMeta::RepairEntitiesInRoom(idVec3 _pos)
{
    idLocationEntity *locEnt = gameLocal.LocationForPoint(_pos);
    if (!locEnt)
    {
        gameLocal.Warning("RepairEntitiesInRoom: cannot find its own location entity (%.1f %.1f %.1f)", _pos.x, _pos.y, _pos.z);
        return false;
    }

    int repairables = 0;
    bool alreadySummoned = false;
    bool summonedRepair = false;
	for (idEntity* ent = gameLocal.repairEntities.Next(); ent != NULL; ent = ent->repairNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden())
			continue;

		if (!ent->needsRepair)
			continue;

        //check for location match.
        idLocationEntity *currentLoc = gameLocal.LocationForEntity(ent);
        if (currentLoc == NULL)
            continue;

        if (currentLoc->entityNumber != locEnt->entityNumber)
            continue;

		ent->repairRequestVerified = true; //Force the item to be "seen" and verified, so that the repair system will mark it as a viable repair candidate.

        repairables++;

		//Has it already been claimed?
		if (HasRepairEntityBeenClaimed(ent))
			continue;

		//If another bot is ALREADY on their way to the repairable, then don't do anything. Basically, we trust that bot will eventually do the job.
		if (IsBotRepairingSomethingNearby(ent))
		{
            alreadySummoned = true;
			continue;
		}

		//First, try to find an existing repairbot that can repair this thing.
		if (SendAvailableRepairBot(ent))
		{
			//Successfully sent the bot.
            alreadySummoned = true;
			continue;
		}

		//See if max repairbots on the field has been reached.
		if (GetRepairBotCount() >= gameLocal.world->spawnArgs.GetInt("maxrepairbots", "4"))
		{
			continue;
		}

		//Activate a repairbot spawner. And assign task to it.
		ActivateClosestRepairHatch(ent);
        summonedRepair = true;
	}

    
    if (summonedRepair) //success
    {
        return REPRESULT_SUCCESS;
    }

    if (repairables <= 0)
    {
        //No repairables found.
        return REPRESULT_ZERO_REPAIRABLES;
    }

    if (alreadySummoned)
    {
        return REPRESULT_ALREADY_REPAIRBOT;
    }

    return REPRESULT_FAIL;

}

bool idMeta::DoesEnemyHaveLOStoRepairable(idEntity *repairableEnt)
{
	//Only do check if player is in the area.
	if (!gameLocal.InPlayerPVS(repairableEnt))
		return false;

	idLocationEntity *repairableLocation = NULL;
	repairableLocation = gameLocal.LocationForEntity(repairableEnt);

	if (repairableLocation == NULL)
	{
		return false;
	}

	//Iterate over the AIs
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0 || entity->team != TEAM_ENEMY)
			continue;

		if (!entity->IsType(idAI::Type))
			continue;

		if (entity->IsType(idGunnerMonster::Type))
		{
			if (static_cast<idGunnerMonster *>(entity)->aiState != AISTATE_IDLE)
			{
				continue;
			}
		}

		//Some things cannot verify repair objects, such as swordfish
		if (!static_cast<idAI*>(entity)->doesRepairVerify)
			continue;
		

		//check if in same room.
		idLocationEntity *aiLocation = NULL;
		aiLocation = gameLocal.LocationForEntity(entity);
		
		if (aiLocation == NULL)
			continue;

		if (aiLocation->entityNumber != repairableLocation->entityNumber)
			continue; //Not in same room.

		//The AI and the repairable are in the same room.
		if (CanAISeeRepairable(repairableEnt, static_cast<idAI *>(entity)))
		{

            if (gameLocal.time > repairVO_timer)
            {
                //The entity has successfully been observed by an AI.
                //Play the VO line on the AI.
                gameLocal.voManager.SayVO(entity, "snd_vo_repairrequest", VO_CATEGORY_BARK);
                repairVO_timer = gameLocal.time + REPAIRMANAGER_VO_COOLDOWNTIME;

				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_repairrequest"), entity->displayName.c_str(), repairableEnt->displayName.c_str()).c_str(), entity->GetPhysics()->GetOrigin() );
            }

			return true;
		}
	}

	return false;
}

bool idMeta::CanAISeeRepairable(idEntity *repairableEnt, idAI *aiEnt)
{
	trace_t tr;
	gameLocal.clip.TracePoint(tr, aiEnt->GetEyePosition(), repairableEnt->GetPhysics()->GetOrigin(), MASK_SOLID, repairableEnt);

	float distanceToEnt = (tr.endpos - repairableEnt->GetPhysics()->GetOrigin()).Length();
	return (distanceToEnt <= 64);
}

void idMeta::ActivateClosestRepairHatch(idEntity * repairableEnt)
{

	idLocationEntity* locRepairableEnt = gameLocal.LocationForEntity(repairableEnt);

	idStaticList<spawnSpot_t, MAX_GENTITIES> candidates;

	//Gather up all the repairhatches in the level.
	for (idEntity* entity = gameLocal.hatchEntities.Next(); entity != NULL; entity = entity->hatchNode.Next())
	{
		if (!entity)
			continue;

		if (entity->IsType(idDozerhatch::Type))
		{
			// SW 26th March 2025: adding functionality to exclude hatches in unreachable rooms (e.g. because of locked doors)
			aasPath_t path;
			if (!entity->GetPhysics() || !repairableEnt->GetPhysics())
			{
				continue; // this shouldn't happen, but it's better to fail silently than to crash
			}
			idLocationEntity* startLocation = gameLocal.LocationForPoint(entity->GetPhysics()->GetOrigin() + (entity->GetPhysics()->GetAxis() * idVec3(0, 0, -32)));
			idLocationEntity* endLocation = locRepairableEnt;
			
			if (startLocation == NULL || endLocation == NULL)
			{
				gameLocal.Warning("Tried to dispatch a repairbot from hatch %s to entity %s, but one of them was not in a valid location.", entity->name.c_str(), repairableEnt->name.c_str());
				continue; // One of these two things is not in a valid location (boooo)
			}

			int distanceModifier = 0;

			// If the locations aren't the same, we need to test that one is actually reachable from the other
			// If the locations *are* the same, we can safely skip this whole bloody palaver
			if (startLocation != endLocation)
			{
				idAAS* aas = gameLocal.GetAAS("aas32_flybot");

				if (!aas)
				{
					continue; // this shouldn't happen
				}

				int startArea = aas->PointAreaNum(startLocation->GetPhysics()->GetOrigin());
				int endArea = aas->PointAreaNum(endLocation->GetPhysics()->GetOrigin());

				if (startArea < 0 || endArea < 0)
				{
					continue; // this also shouldn't happen
				}

				idReachability* reachability;
				int travelTime;
				if (!gameLocal.GetAAS("aas32_flybot")->RouteToGoalArea(startArea, startLocation->GetPhysics()->GetOrigin(), endArea, TFL_WALK | TFL_AIR | TFL_FLY, travelTime, &reachability))
				{
					continue; // Couldn't find a path
				}
			}
			else
			{
				//Hatch is in the same room. So, we give a distance "bonus" modifier to it, so that it gets prioritized.
				distanceModifier = -3000;
			}

			spawnSpot_t newcandidate;
			newcandidate.ent = entity;
			newcandidate.dist = (repairableEnt->GetPhysics()->GetOrigin() - entity->GetPhysics()->GetOrigin()).LengthFast() + distanceModifier;
			candidates.Append(newcandidate);
		}
	}

	if (candidates.Num() <= 0)
	{
		return; //None found. Exit here.
	}
	else if (candidates.Num() == 1)
	{
		//There's only one repairhatch. Spawn the bot.
		static_cast<idDozerhatch*>(candidates[0].ent)->StartSpawnSequence(repairableEnt, BOT_REPAIRBOT);
		return;
	}

	//Multiple hatches. Sort them by distance, use the closest hatch.
	qsort((void *)candidates.Ptr(), candidates.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Nearest);
	static_cast<idDozerhatch*>(candidates[0].ent)->StartSpawnSequence(repairableEnt, BOT_REPAIRBOT);
}

int idMeta::GetSecurityBotsInRoom(idEntity* targetEnt)
{
	idLocationEntity* originLocation = gameLocal.LocationForEntity(targetEnt);

	if (!originLocation)
		return -1; //player has no location? Return -1 error.

	int tally = 0;
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (entity == nullptr)
			continue;

		if (entity->IsHidden() || entity->health <= 0 || !entity->IsType(idAI_Spearbot::Type))
			continue;

		idLocationEntity* botLocation = gameLocal.LocationForEntity(entity);
		if (botLocation)
		{
			if (botLocation->entityNumber == originLocation->entityNumber)
			{
				tally++;
			}
		}
	}

	return tally;
}

//Spawn spearfish IN SAME ROOM as a given entity. This is used for the combat alert spawning. This is similar to SpawnSecurityBot, but ONLY cares about the same room. 
bool idMeta::SpawnSecurityBotInRoom(idEntity* targetEnt)
{
	//Make a big list of all the hatches in the level.
	idStaticList<spawnSpot_t, MAX_GENTITIES> candidates;
	for (idEntity* entity = gameLocal.hatchEntities.Next(); entity != NULL; entity = entity->hatchNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idDozerhatch::Type))
			continue;

		spawnSpot_t newcandidate;
		newcandidate.ent = entity;
		candidates.Append(newcandidate);
	}

	if (candidates.Num() <= 0)
		return false;

	//Ok, we now have a list of all hatches.
	//See if there's any in the SAME ROOM.
	int bestSameRoomIdx = -1;
	float closestSameRoomDist = 9999;
	idLocationEntity* originLocation = gameLocal.LocationForEntity(targetEnt);

	if (originLocation != NULL)
	{
		for (int i = 0; i < candidates.Num(); i++)
		{
			idLocationEntity* locationEnt = gameLocal.LocationForEntity(candidates[i].ent);

			if (locationEnt == NULL)
				continue;

			if (locationEnt->entityNumber != originLocation->entityNumber)
				continue;

			//We have a hatch that is in the SAME ROOM.
			float dist = (candidates[i].ent->GetPhysics()->GetOrigin() - targetEnt->GetPhysics()->GetOrigin()).LengthFast();
			if (dist < closestSameRoomDist)
			{
				//Find the closest-distance hatch in same room.
				closestSameRoomDist = dist;
				bestSameRoomIdx = i;
			}
		}

		if (bestSameRoomIdx >= 0)
		{
			//Spawn a bot in the same room.
			static_cast<idDozerhatch*>(candidates[bestSameRoomIdx].ent)->StartSpawnSequence(NULL, BOT_SECURITYBOT);
			return true;
		}
	}

	return false;
}

//Spawn a spearfish.
bool idMeta::SpawnSecurityBot(idVec3 _roomPos)
{
	//Make a big list of all the hatches in the level.
	idStaticList<spawnSpot_t, MAX_GENTITIES> candidates;
	for (idEntity* entity = gameLocal.hatchEntities.Next(); entity != NULL; entity = entity->hatchNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idDozerhatch::Type))
			continue;
		
		spawnSpot_t newcandidate;
		newcandidate.ent = entity;
		newcandidate.dist = (_roomPos - entity->GetPhysics()->GetOrigin()).LengthFast();
		candidates.Append(newcandidate);
	}

	if (candidates.Num() <= 0)
		return false;

	//Ok, we now have a list of all hatches.
	//First, we try to see if there's any in the SAME ROOM.
	int bestSameRoomIdx = -1;
	float closestSameRoomDist = 9999;
	idLocationEntity *originLocation = gameLocal.LocationForPoint(_roomPos);
	
	if (originLocation != NULL)
	{
		for (int i = 0; i < candidates.Num(); i++)
		{
			idLocationEntity *locationEnt = gameLocal.LocationForEntity(candidates[i].ent);

			if (locationEnt == NULL)
				continue;

			if (locationEnt->entityNumber != originLocation->entityNumber)
				continue;

			//We have a hatch that is in the SAME ROOM.
			float dist = (candidates[i].ent->GetPhysics()->GetOrigin() - _roomPos).LengthFast();
			if (dist < closestSameRoomDist)
			{
				//Find the closest-distance hatch.
				closestSameRoomDist = dist;
				bestSameRoomIdx = i;
			}
		}

		if (bestSameRoomIdx >= 0)
		{
			//Spawn a bot in the same room.
			static_cast<idDozerhatch*>(candidates[bestSameRoomIdx].ent)->StartSpawnSequence(NULL, BOT_SECURITYBOT);
			return true;
		}
	}

	//Ok, couldn't spawn a bot in the same room.
	//Fall back to just spawning at the closest hatch.
	qsort((void *)candidates.Ptr(), candidates.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Nearest);
	static_cast<idDozerhatch*>(candidates[0].ent)->StartSpawnSequence(NULL, BOT_SECURITYBOT);
	return true;
}

bool idMeta::HasRepairEntityBeenClaimed(idEntity * repairableEnt)
{
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idAI_Repairbot::Type))
			continue;

		if (entity->health <= 0)
			continue;

		if (static_cast<idAI_Repairbot*>(entity)->repairEnt.IsValid())
		{
			if (static_cast<idAI_Repairbot*>(entity)->repairEnt.GetEntity() == repairableEnt)
				return true;
		}
	}

	return false;
}

int idMeta::GetRepairBotCount()
{
	int count = 0;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;
		
		if (!entity->IsType(idAI_Repairbot::Type))
			continue;

		count++;
	}

	return count;
}

bool idMeta::SendAvailableRepairBot(idEntity * repairableEnt)
{
	idStaticList<spawnSpot_t, MAX_GENTITIES> candidates;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idAI_Repairbot::Type))
			continue;

		if (static_cast<idAI_Repairbot*>(entity)->aiState == 0) //0 = REPAIRBOT_IDLE
		{
			spawnSpot_t newcandidate;
			newcandidate.ent = entity;
			newcandidate.dist = (repairableEnt->GetPhysics()->GetOrigin() - entity->GetPhysics()->GetOrigin()).LengthFast();
			candidates.Append(newcandidate);
		}
	}

	if (candidates.Num() == 1)
	{
		//only one. just return it.
		if (static_cast<idAI_Repairbot*>(candidates[0].ent)->SetRepairTask(repairableEnt))
		{
			return true;
		}
	}
	else
	{
		//Ok, we have a handful of candidates. Attempt to assign the one geographically closest to the repairable ent.
		qsort((void *)candidates.Ptr(), candidates.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Nearest);

		for (int i = 0; i < candidates.Num(); i++)
		{
			if (static_cast<idAI_Repairbot*>(candidates[i].ent)->SetRepairTask(repairableEnt))
			{
				return true;
			}
		}
	}

	//No candidates.
	return false;
}

#define REPAIR_SKIPSEND_DISTANCETHRESHOLD 512 //If another bot is already on its way to repair something there, don't request another bot. This is to prevent a parade of bots.

bool idMeta::IsBotRepairingSomethingNearby(idEntity * repairableEnt)
{
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idAI_Repairbot::Type))
			continue;

		if (static_cast<idAI_Repairbot*>(entity)->aiState == 1 || static_cast<idAI_Repairbot*>(entity)->aiState == 2)  //REPAIRBOT_SEEKINGREPAIR, REPAIRBOT_REPAIRING
		{
			//Ok, this bot is on its way to repair something.
			if (static_cast<idAI_Repairbot*>(entity)->repairEnt.IsValid())
			{
				//Compare distance of "bot's currently assigned repairable" to "the new repairable order"
				float distanceBetweenRepairables = (static_cast<idAI_Repairbot*>(entity)->repairEnt.GetEntity()->GetPhysics()->GetOrigin() - repairableEnt->GetPhysics()->GetOrigin()).LengthFast();

				if (distanceBetweenRepairables < REPAIR_SKIPSEND_DISTANCETHRESHOLD)
				{
					return true;
				}
			}			
		}
	}

	for (idEntity* entity = gameLocal.hatchEntities.Next(); entity != NULL; entity = entity->hatchNode.Next())
	{
		if (!entity)
			continue;

		if (entity->IsType(idDozerhatch::Type))
		{
			//if hatch is currently spawning something, then we skip.
			if (static_cast<idDozerhatch*>(entity)->IsCurrentlySpawning())
			{
				return true;
			}
		}
	}

	return false;
}



//Starts ventpurge. Stages:
// - walkie talkie dispatch
// - ventpurge chargeup
// - ventpurge blast
bool idMeta::StartVentPurge(idEntity *interestpoint)
{
	if (ventpurgeState != VENTPURGESTATE_IDLE || !gameLocal.world->spawnArgs.GetBool("meta_ventpurge", "1"))
		return false;

	ventpurgeState = VENTPURGESTATE_DISPATCH;
	ventpurgeTimer = gameLocal.time + VENTPURGE_DISPATCHTIME;

	//Make the dispatch say its line.
	//BC 2-15-2025: determine if there's a specific voiceprint we should use.
	idStr soundCue = "snd_dispatch_a_ventpurge";
	if (interestpoint != nullptr)
	{
		if (interestpoint->IsType(idInterestPoint::Type))
		{
			if (static_cast<idInterestPoint*>(interestpoint)->claimant.IsValid())
			{
				int _voiceprint = static_cast<idInterestPoint*>(interestpoint)->claimant.GetEntity()->spawnArgs.GetInt("voiceprint");
				if (_voiceprint == VOICEPRINT_BOSS)
				{
					soundCue = "snd_dispatch_boss_ventpurge";
				}
				else if (_voiceprint == VOICEPRINT_B)
				{
					soundCue = "snd_dispatch_b_ventpurge";
				}
				else
				{
					soundCue = "snd_dispatch_a_ventpurge";
				}

				dispatchVoiceprint = _voiceprint;
			}
		}
	}

	this->StartSound(soundCue.c_str(), SND_CHANNEL_VOICE);

	enemyThinksPlayerIsInVentOrAirlock = true;

	return true;
}

void idMeta::UpdateVentpurge()
{
	if (ventpurgeState == VENTPURGESTATE_DISPATCH)
	{
		if (gameLocal.time >= ventpurgeTimer)
		{
			//PA system.
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ventcleanse");

			//Make the purge signage pop out.
			for (idEntity* entity = gameLocal.ventdoorEntities.Next(); entity != NULL; entity = entity->ventdoorNode.Next())
			{
				if (!entity)
					continue;

				if (!entity->IsType(idVentdoor::Type))
					continue;

				static_cast<idVentdoor *>(entity)->SetPurgeSign(true); //Make sign appear.
			}

			

			ventpurgeState = VENTPURGESTATE_ANNOUNCE;
			ventpurgeTimer = gameLocal.time + VENTPURGE_ANNOUNCETIME;
		}
	}
	if (ventpurgeState == VENTPURGESTATE_ANNOUNCE)
	{
		//doing the announcement vo.

		if (gameLocal.time >= ventpurgeTimer)
		{
			//Announcement is done. Start the countdown.
			ventpurgeState = VENTPURGESTATE_CHARGEUP;
			ventpurgeTimer = gameLocal.time + VENTPURGE_CHARGEUPTIME;

			//Play the sound on a confinedtrigger.
			PlayVentSound("snd_ventcharge");

			gameLocal.AddEventLog("#str_def_gameplay_vent_purge", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), true, EL_ALERT);
		}
	}
	else if (ventpurgeState == VENTPURGESTATE_CHARGEUP)
	{
		//charge up sequence.

		if (gameLocal.time >= ventpurgeTimer)
		{
			ventpurgeState = VENTPURGESTATE_PURGING; //During this stage, will make all confined triggers inflict ventpurge damage.
			ventpurgeTimer = gameLocal.time + 100;

			PlayVentSound("snd_ventpurge");

			//Do the purge particle fx here.
			for (idEntity* entity = gameLocal.confinedEntities.Next(); entity != NULL; entity = entity->confinedNode.Next())
			{
				idVec3 dirToTrigger, particlePos;
				float facingResult, distToPlayer;

				if (!entity)
					continue;

				if (entity == NULL)
					continue;

				if (!entity->IsType(idTrigger_confinedarea::Type) || !entity->spawnArgs.GetBool("purge"))
					continue;
				
				//check if trigger is in front of player.
				dirToTrigger = entity->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
				facingResult = DotProduct(dirToTrigger, gameLocal.GetLocalPlayer()->viewAngles.ToForward());

				if (facingResult < 0)
					continue; //Behind player. ignore it.

				distToPlayer = (entity->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();

				if (distToPlayer > 1024)
					continue;

				particlePos = entity->GetPhysics()->GetAbsBounds().GetCenter();
				gameLocal.DoParticle("explosion_ventpurge.prt", particlePos);
			}

			//Particle fx for the ventdoors.
			for (idEntity* entity = gameLocal.ventdoorEntities.Next(); entity != NULL; entity = entity->ventdoorNode.Next())
			{
				//idVec3 forward;
				idAngles particleAng;
				float distToPlayer;

				if (!entity)
					continue;

				if (entity == NULL)
					continue;

				//entity->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

				distToPlayer = (entity->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();

				if (distToPlayer > 1024)
					continue;

				particleAng = entity->GetPhysics()->GetAxis().ToAngles();

				gameLocal.DoParticle("vent_purgesmoke.prt", entity->GetPhysics()->GetOrigin(), particleAng.ToForward());
			}

			//logic for inflicting damage on objects that are touching a ventpurge trigger.
			//We used to call it in bc_trigger_confinedarea.cpp Event_Touch() but the problem was that it would damage an entity multiple times if it was touching multiple triggers.
			if (1)
			{
				for (idEntity* ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next())
				{
					if (!ent || !ent->fl.takedamage || ent->fl.hidden || ent->fl.notarget || ent->fl.isDormant)
					{
						continue;
					}

					//Ok, we have an entity that might be valid for taking damage.
					//See if ent is touching a confined purge trigger.

					if (gameLocal.GetPositionIsInsideConfinedspace(ent->GetPhysics()->GetOrigin()))
					{
						ent->Damage(NULL, NULL, vec3_zero, "damage_ventpurge", 1.0f, 0);

						if (ent->IsType(idPlayer::Type))
						{
							gameLocal.DoParticle("explosion_ventpurge.prt", gameLocal.GetLocalPlayer()->firstPersonViewOrigin + gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 16);
						}
					}

					
				}
			}
		}
	}
	else if (ventpurgeState == VENTPURGESTATE_PURGING)
	{
		if (gameLocal.time >= ventpurgeTimer)
		{
			ventpurgeState = VENTPURGESTATE_ALLCLEAR_DELAY;
			ventpurgeTimer = gameLocal.time + VENTPURGE_ALLCLEARANNOUNCE_DELAYTIME;

			//Make the purge signage go away.
			for (idEntity* entity = gameLocal.ventdoorEntities.Next(); entity != NULL; entity = entity->ventdoorNode.Next())
			{
				if (!entity)
					continue;

				if (!entity->IsType(idVentdoor::Type))
					continue;

				static_cast<idVentdoor *>(entity)->SetPurgeSign(false); //Make sign retract back in.
			}

			
		}
	}
	else if (ventpurgeState == VENTPURGESTATE_ALLCLEAR_DELAY)
	{
		//Purge happened, am currently doing a delay before the all-clear VO.		

		if (gameLocal.time >= ventpurgeTimer)
		{
			ventpurgeState = VENTPURGESTATE_IDLE;
			StartAllClearSequence(1000);
		}
	}	
}


//After a vent purge or airlock lockdown, we do an all-clear sequence where the enemy fictionally thinks "hey, the purge probably killed the player. Let's lift the security lockdown."
void idMeta::StartAllClearSequence(int delayTime, int _voiceprint)
{
	allclearState = ALLCLEAR_DELAY;
	allclearTimer = gameLocal.time + delayTime;

	dispatchVoiceprint = _voiceprint;
}

//This gets called when the player uses the walkie talkie to call off a search.
bool idMeta::StartPlayerWalkietalkieSequence(int *_voLength)
{
	*_voLength = 0;

	if (allclearState != ALLCLEAR_DORMANT)
		return false;

	allclearState = ALLCLEAR_PLAYERWALKIETALKIE;	

	//BC 3-3-2025: checkin now uses vo system so that it doesnt get interrupted
	int len = gameLocal.voManager.SayVO(gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer()->GetInjured() ? "snd_vo_walkie_hijack_injured" : "snd_allclear", VO_CATEGORY_NARRATIVE);
	//gameLocal.GetLocalPlayer()->StartSound( gameLocal.GetLocalPlayer()->GetInjured() ? "snd_vo_walkie_hijack_injured" : "snd_allclear", SND_CHANNEL_VOICE, 0, false, &len); //play VO line on player.
	allclearTimer = gameLocal.time + len + ALLCLEAR_GAP_DURATION;

	if (radioCheckinEnt != NULL)
	{
		//If radio checkin is happening, make it stop.
		radioCheckinEnt->StopCheckin();
		radioCheckinEnt->ResetCooldown();
	}

	*_voLength = len;

	return true;
}

void idMeta::UpdateAllClearSequence()
{
	if (allclearState == ALLCLEAR_PLAYERWALKIETALKIE)
	{
		if (gameLocal.time >= allclearTimer)
		{
			


			allclearState = ALLCLEAR_DORMANT;
			GotoCombatIdleState();

			if (radioCheckinEnt != NULL)
			{
				radioCheckinEnt->ResetCooldown(); //don't do the radio checkin for a while.
			}

			//clear out the LKP.
			SetLKPVisible(false);
		}
	}
	else if (allclearState == ALLCLEAR_DELAY)
	{
		if (gameLocal.time >= allclearTimer)
		{
			if (enemyThinksPlayerIsInVentOrAirlock)
			{
				allclearState = ALLCLEAR_ANNOUNCING;

				//BC 2-15-2025: purge complete dispatch vo now uses same voiceprint as the voiceprint who initially activated the purge.
				idStr soundcue = "snd_dispatch_a_purgecomplete";
				if (dispatchVoiceprint == VOICEPRINT_BOSS)
				{
					soundcue = "snd_dispatch_boss_purgecomplete";
				}
				else if (dispatchVoiceprint == VOICEPRINT_B)
				{
					soundcue = "snd_dispatch_b_purgecomplete";
				}
				else
				{
					soundcue = "snd_dispatch_a_purgecomplete";
				}

				this->StartSound(soundcue.c_str(), SND_CHANNEL_VOICE2);
				allclearTimer = gameLocal.time + ALLCLEAR_ANNOUNCE_VO_TIME; //TODO: RENAME THIS
			}
			else
			{
				allclearState = ALLCLEAR_DORMANT;
			}
		}
	}
	else if (allclearState == ALLCLEAR_ANNOUNCING)
	{
		if (!enemyThinksPlayerIsInVentOrAirlock)
		{
			StopSound(SND_CHANNEL_VOICE2);
			allclearState = ALLCLEAR_DORMANT;

			if (radioCheckinEnt != NULL)
			{
				radioCheckinEnt->ResetCooldown(); //don't do the radio checkin for a while.
			}
			return;
		}

		if (gameLocal.time >= allclearTimer)
		{
			allclearState = ALLCLEAR_DORMANT; //All done.
			GotoCombatIdleState(); //Exit the search state.

			gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_thinkdead");

			if (radioCheckinEnt != NULL)
			{
				radioCheckinEnt->ResetCooldown(); //don't do the radio checkin for a while.
			}
		}
	}
}

void idMeta::SetAllClearInterrupt()
{
	//if we're in the all-clear sequence, call this if enemy sees/interacts with player. Immediately halts the all-clear sequence.
	if (allclearState != ALLCLEAR_DORMANT)
	{
		if (allclearState == ALLCLEAR_PLAYERWALKIETALKIE)
		{
			gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_VOICE); //Stop the player from saying the walkie talkie stuff.
		}

		if (allclearState == ALLCLEAR_ANNOUNCING)
		{
			StopSound(SND_CHANNEL_VOICE2); //if playing the all-clear audio, stop it immediately.			
		}

		allclearState = ALLCLEAR_DORMANT;
	}
}

void idMeta::PlayVentSound(const char *soundname)
{
	//Just play the sound on the first confined triggger you find.
	idEntity *confinedTrigger = gameLocal.confinedEntities.Next();

	if (confinedTrigger)
	{
		confinedTrigger->StartSound(soundname, SND_CHANNEL_ANY, 0, false, NULL);
	}
}

bool idMeta::IsPurging()
{
	return (ventpurgeState == VENTPURGESTATE_PURGING);
}

bool idMeta::GetSignallampActive()
{
	if (signallampEntity.IsValid())
	{
		if (signallampEntity.GetEntity() != NULL)
		{
			idThread::ReturnInt(1);
			return true;
		}
	}

	idThread::ReturnInt(0);
	return false;
}



#define UPGRADECARGO_UNLOCKS_PER_STAGE 1

void idMeta::InitializeUpgradeCargos()
{
	//First, gather all the upgrade definitions.
	idStr rawloadout = spawnArgs.GetString("def_upgradecargos", "");
	rawloadout.StripTrailingWhitespace();
	rawloadout.StripLeading(' ');

	if (rawloadout.Length() <= 0)
	{
		gameLocal.Error("failed to find 'def_upgradecargos' in target_meta.");
	}

	idStrList upgradeList;
	for (int i = rawloadout.Length(); i >= 0; i--)
	{
		if (rawloadout[i] == ',')
		{
			upgradeList.Append(rawloadout.Mid(i + 1, rawloadout.Length()).c_str());
			rawloadout = rawloadout.Left(i); //Truncate the string and continue.
		}
		else if (i <= 0)
		{
			upgradeList.Append(rawloadout.Mid(0, rawloadout.Length()).c_str());
		}
	}

	//Ok, we now have a list of the upgrades we want to distribute amongst the upgrade boxes.

	//Randomize the list order.
	idRandom newRandom(time(NULL)); //at game start, gameLocal.random always returns the same values. So create a new random seed here.
	for (int i = 0; i < upgradeList.Num(); i++)
	{
		int randIdx = newRandom.RandomInt(upgradeList.Num());
		idStr randValue = upgradeList[randIdx];

		idStr curValue = upgradeList[i];

		//swap them.
		upgradeList[i] = randValue.c_str();
		upgradeList[randIdx] = curValue.c_str();
	}
	
	if (gameLocal.upgradecargoEntities.Num() > upgradeList.Num())
	{
		gameLocal.Error("There are not enough 'upgradecargo_*' definitions to populate the 'env_upgradecargo' entities.\n");
	}

	//Distribute the upgrades amongst all the upgrade boxes.
	for (int i = 0; i < gameLocal.upgradecargoEntities.Num(); i++)
	{
		if (!static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->SetInfo(upgradeList[i].c_str()))
		{
			gameLocal.Error("Unable to load upgrade definition '%s'.", upgradeList[i].c_str());
		}
	}
}

void idMeta::DoUpgradeCargoUnlocks()
{
	//First, check if there are any boxes that are currently already available. If so, make them dormant and reward them.
	for (int i = 0; i < gameLocal.upgradecargoEntities.Num(); i++)
	{
		if (static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->IsAvailable())
		{
			//TODO: reward these cargos to the AI faction.

			static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->SetDormant();
		}
	}


	//Find two boxes to unlock.
	idList<int> cargoCandidates;

	for (int i = 0; i < gameLocal.upgradecargoEntities.Num(); i++)
	{
		if (!static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->IsLocked())
			continue;

		//we have a locked cargobox.
		cargoCandidates.Append(gameLocal.upgradecargoEntities[i]->entityNumber);
	}

	if (cargoCandidates.Num() <= 0)
	{
		return; //no more cargo boxes to unlock..... exit here.
	}

	if (cargoCandidates.Num() <= UPGRADECARGO_UNLOCKS_PER_STAGE)
	{
		//We have the bare minimum. Unlock all of them.
		for (int i = 0; i < cargoCandidates.Num(); i++)
		{
			int index = cargoCandidates[i];
			static_cast<idUpgradecargo *>(gameLocal.entities[index])->SetAvailable();
		}
		return;
	}
	
	//Ok, we have a list of candidates. Randomly choose 2 to unlock. The way we do this is randomize the list, then select the first XX entries from the list. This prevents us from randomly choosing the same thing twice.

	
	idRandom newRandom(time(NULL)); //at game start, gameLocal.random always returns the same values. So create a new random seed here.

	//Randomize the list. This function just swaps values around randomly.
	for (int i = 0; i < cargoCandidates.Num(); i++)
	{
		int j, x, jValue, xValue;

		//Randomly select one of the values.
		j = newRandom.RandomInt(cargoCandidates.Num());
		jValue = cargoCandidates[j];

		//Get the current value.
		x = cargoCandidates[i];
		xValue = cargoCandidates[i];

		//Swap them.
		cargoCandidates[i] = jValue;
		cargoCandidates[j] = xValue;
	}
	
	//Unlock the first XX entries from the list.
	for (int i = 0; i < UPGRADECARGO_UNLOCKS_PER_STAGE; i++)
	{
		int index = cargoCandidates[i];
		static_cast<idUpgradecargo *>(gameLocal.entities[index])->SetAvailable();
	}


	//Update the manifest computers.
	for (int i = 0; i <  gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (!gameLocal.entities[i]->IsType(idManifestStation::Type))
			continue;

		static_cast<idManifestStation *>(gameLocal.entities[i])->UpdateInfos();
	}

}

void idMeta::InitializeItemSpawns()
{
	//First, gather all the item definitions into the hash table.
	idRandom newRandom(time(NULL)); //at game start, gameLocal.random always returns the same values. So create a new random seed here.

	for (int i = 0; i < spawnArgs.GetNumKeyVals(); ++i)
	{
		const idKeyValue* kv = spawnArgs.GetKeyVal(i);
		if (kv->GetKey().Find("def_itemspawns") != -1)
		{
			idStr rawloadout = kv->GetValue();
			rawloadout.StripTrailingWhitespace();
			rawloadout.StripLeading(' ');

			idStrList upgradeList;
			for (int j = rawloadout.Length(); j >= 0; j--)
			{
				if (rawloadout[j] == ',')
				{
					upgradeList.Append(rawloadout.Mid(j + 1, rawloadout.Length()).c_str());
					rawloadout = rawloadout.Left(j); //Truncate the string and continue.
				}
				else if (j <= 0)
				{
					upgradeList.Append(rawloadout.Mid(0, rawloadout.Length()).c_str());
				}
			}

			//Randomize the list order.
			for (int j = 0; j < upgradeList.Num(); j++)
			{
				int randIdx = newRandom.RandomInt(upgradeList.Num());
				idStr randValue = upgradeList[randIdx];
				idStr curValue = upgradeList[j];
				upgradeList[j] = randValue.c_str();
				upgradeList[randIdx] = curValue.c_str();
			}

			// Figure out key for hash table
			idStr spawnKey = kv->GetKey();
			idStr key = "*";
			if (spawnKey.Length() > 14)
			{
				spawnKey.StripLeading("def_itemspawns_");
				key = spawnKey;
			}

			itemdropsTable.Set(key, upgradeList);
		}
	}
	
	idList<int> spawnPoints;

	idEntity *ent;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent || ent == NULL)
			continue;

		if (idStr::Cmp(ent->spawnArgs.GetString("classname"), "target_itemspawner") == 0)
		{
			spawnPoints.Append(ent->entityNumber);
		}
	}

	if (spawnPoints.Num() <= 0)
		return; //no spawn points. that's ok, maybe it's a test map or something.
	
	//randomize this list of spawnpoints.
	for (int i = 0; i < spawnPoints.Num(); i++)
	{
		int randIdx = newRandom.RandomInt(spawnPoints.Num());
		int randValue = spawnPoints[randIdx];
		int curValue = spawnPoints[i];
		spawnPoints[i] = randValue;
		spawnPoints[randIdx] = curValue;
	}


	if (developer.GetInteger() > 0)
	{
		common->Printf("\n\n-------- START ITEMSPAWNER DEBUG --------\n");
	}

	int totalSpawned = 0;

	//Spawn the items.
	for (int i = 0; i < itemdropsTable.Num(); ++i)
	{
		idStr itemType;
		idStrList upgradeList;
		itemdropsTable.GetIndex(i, &itemType, &upgradeList);
		int k = 0;
		for (int j = 0; j < upgradeList.Num(); ++j)
		{
			if (k >= spawnPoints.Num())
				break;

			bool spawned = false;
			for (; k < spawnPoints.Num() && !spawned; ++k)
			{
				idEntity *spawnEnt = gameLocal.entities[spawnPoints[k]];
				idStr spawnType = spawnEnt->spawnArgs.GetString("item_spawn_type", "*");
				if (spawnType != itemType)
				{
					continue;
				}
				const idDeclEntityDef *itemDef;

				itemDef = gameLocal.FindEntityDef(upgradeList[j], false);

				if (!itemDef)
				{
					gameLocal.Error("InitializeItemSpawns(): failed to find def for '%s'.", upgradeList[j].c_str());
				}

				idEntity *itemEnt;
				gameLocal.SpawnEntityDef(itemDef->dict, &itemEnt, false);

				if (!itemEnt)
				{
					gameLocal.Error("InitializeItemSpawns(): found def but failed to spawn '%s'.", upgradeList[j].c_str());
				}

				//Spawn it.

				//Place it directly on the floor so that it doesn't alert the AI.
				trace_t floorTr;
				gameLocal.clip.TracePoint(floorTr, spawnEnt->GetPhysics()->GetOrigin(), spawnEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, -64), MASK_SOLID, NULL); //Get floor position.
				idBounds itemBounds = itemEnt->GetPhysics()->GetBounds();
				idVec3 spawnPos = floorTr.endpos + idVec3(0, 0, -itemBounds[0].z + 0.2f);
				itemEnt->SetOrigin(spawnPos);

				//face same angle as the itemspawner.
				itemEnt->SetAxis(spawnEnt->GetPhysics()->GetAxis());

				if (developer.GetInteger() > 0)
				{
					//do debug info.
					idLocationEntity *locationEntity = NULL;

					locationEntity = gameLocal.LocationForPoint(spawnPos);

					common->Printf("%2d:%-30s %-18s %-5.0f %-5.0f %-5.0f\n", i + 1, upgradeList[i].c_str(),
						(locationEntity != NULL) ? locationEntity->GetLocation() : "???",
						spawnPos.x, spawnPos.y, spawnPos.z);

					totalSpawned++;
				}

				spawned = true;
			}
		}
	}

	if (developer.GetInteger() > 0)
	{
		common->Printf("-------- END ITEMSPAWNER DEBUG. Items spawned: %d --------\n\n\n", totalSpawned);
	}

}





idEntity * idMeta::SpawnIdleTask(idEntity *ownerEnt, idVec3 position, const char *idletaskName)
{
	idEntity				*idletaskEnt;
	const idDeclEntityDef	*idleDef;

	if (idletaskName[0] == '/0') //if def is empty, then skip.
		return NULL;

	idleDef = gameLocal.FindEntityDef(idletaskName);

	if (!idleDef)
	{
		gameLocal.Error("SpawnIdleTask() failed to find entity def: '%s'\n", idletaskName);
		return NULL;
	}

	gameLocal.SpawnEntityDef(idleDef->dict, &idletaskEnt, false);

	if (!idletaskEnt)
	{
		gameLocal.Error("SpawnIdleTask() found def but failed to spawn entity: '%s'\n", idletaskName);
		return NULL;
	}

	idletaskEnt->SetOrigin(position);

	static_cast<idIdleTask *>(idletaskEnt)->assignedOwner = ownerEnt;

	return idletaskEnt;
}

void idMeta::UpdateIdletasks()
{
	if (gameLocal.time < idletaskTimer)
		return;

	idletaskTimer = gameLocal.time + IDLETASK_UPDATEINTERVAL;

	//Iterate over all the idle task entities.
	for (idEntity* entity = gameLocal.idletaskEntities.Next(); entity != NULL; entity = entity->idletaskNode.Next())
	{
		if (!entity)
			continue;

		if (entity->IsHidden())
			continue;

		idIdleTask *idletask = static_cast<idIdleTask *>(entity);

		if (!idletask)
			continue;

		if (idletask->IsClaimed())
		{
			//Ok, this task is claimed. BUT, we want to make sure it's claimed by someone who is STILL ALIVE and EXISTS and is NOT IN COMBAT.

			if (!idletask->assignedActor.IsValid())
			{
				//assigned actor is no longer valid. Set assigned actor to be null.
				idletask->assignedActor = NULL;
			}
			else
			{
				if (idletask->assignedActor.GetEntity()->health <= 0)
				{
					idletask->assignedActor = NULL; //assigned actor is DEAD. Set assigned actor to be null.
				}
				else if (idletask->assignedActor.GetEntity()->IsType(idAI::Type))
				{
					if (static_cast<idAI*>(idletask->assignedActor.GetEntity())->aiState != AISTATE_IDLE)
					{
						idletask->assignedActor = NULL; //assigned actor is NOT IDLE. Set assigned actor to be null.

						if (idletask->spawnArgs.GetBool("delete_when_interrupted", "0"))
						{
							idletask->PostEventMS(&EV_Remove, 0);
						}
					}
				}
			}		

			continue;
		}
		
		//Ok. we have an idletask that is UNCLAIMED and available for use.
		idEntity *closestActor = GetClosestIdleActor(idletask);

		if (closestActor == NULL)
			continue; //Failed to find someone to assign to this task. That's ok. We move on, and hope for the best next time.

		idAI *actor = static_cast<idAI *>(closestActor);
		idletask->SetActor(actor);
	}
}

idEntity *idMeta::GetClosestIdleActor(idIdleTask *idleTask)
{
	for (idEntity* aiEnt = gameLocal.aimAssistEntities.Next(); aiEnt != NULL; aiEnt = aiEnt->aimAssistNode.Next())
	{
		if (!aiEnt || aiEnt->IsHidden() || aiEnt->health <= 0 || !aiEnt->IsType(idAI::Type) || aiEnt->team != TEAM_ENEMY)
			continue;

		if (static_cast<idAI*>(aiEnt)->aiState != AISTATE_IDLE)
			continue;

		if (!aiEnt->spawnArgs.GetBool("has_idlenodes","0"))
			continue;

		if (idleTask->spawnArgs.GetBool("onlyif_hurt", "0"))
		{
			//this task is only available to actors who are hurt.
			if (aiEnt->health >= aiEnt->maxHealth)
				continue; //actor is at max health. not eligible, so skip this actor.
		}

		bool isAlreadyAssigned = false;
		//Verify whether this actor is already assigned to a idletask.
		for (idEntity* entity = gameLocal.idletaskEntities.Next(); entity != NULL; entity = entity->idletaskNode.Next())
		{
			if (!entity)
				continue;

			if (static_cast<idIdleTask *>(entity)->IsClaimed())
			{
				if (static_cast<idIdleTask *>(entity)->assignedActor.IsValid())
				{
					if (static_cast<idIdleTask *>(entity)->assignedActor.GetEntity() == aiEnt)
					{
						isAlreadyAssigned = true;
					}
				}
			}
		}

		if (isAlreadyAssigned)
			continue;

		return aiEnt;
	}

	return NULL;
}

//int idMeta::GetMetaState()
//{
//	return metaState;
//}

int idMeta::GetCombatState()
{
	return combatMetastate;
}

//Tell brittlefracture windows what DOORS to close when the window shatters.
void idMeta::SetupWindowseals()
{
// 	//Generate a list of all the brittlefracture windows in the level.
// 	idList<int> brittleWindows;
// 	for (int i = 0; i < gameLocal.num_entities; i++)
// 	{
// 		if (!gameLocal.entities[i])
// 			continue;
// 
// 		if (!gameLocal.entities[i]->IsType(idBrittleFracture::Type))
// 			continue;
// 
// 		brittleWindows.Append(i);
// 	}
// 
// 	if (brittleWindows.Num() <= 0)
// 		return; //if no brittlefracture windows in level, then exit here.
// 
// 	//Now assign doors to the appropriate windows.
// 	for (int i = 0; i < gameLocal.num_entities; i++)
// 	{
// 		if (!gameLocal.entities[i])
// 			continue;
// 
// 		if (!gameLocal.entities[i]->IsType(idDoor::Type))
// 			continue;
// 
// 		if (!gameLocal.entities[i]->spawnArgs.GetBool("vacuumlock", "0"))
// 			continue;
// 
// 		//Find what room(s) this door is associated with. Assume doors are always connected to 2 rooms, so we check the front and back of the door.
// 		idVec3 forward, up;
// 		gameLocal.entities[i]->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
// 		idVec3* doorCheckPositions = new idVec3[2] //Array of positions on either side of door.
// 		{
// 			gameLocal.entities[i]->GetPhysics()->GetOrigin() + (up * 4) + (forward * 16), //front of door.
// 			gameLocal.entities[i]->GetPhysics()->GetOrigin() + (up * 4) + (forward * -16) //behind door.
// 		};
// 		
// 		for (int x = 0; x < 2; x++)
// 		{
// 			idLocationEntity *locationEntity = NULL;
// 			locationEntity = gameLocal.LocationForPoint(doorCheckPositions[x]);
// 			if (locationEntity)
// 			{
// 				//Iterate through all the windows. Check each door against each window.
// 		
// 				for (int windowIdx = 0; windowIdx < brittleWindows.Num(); windowIdx++)
// 				{
// 					int brittleIndex = brittleWindows[windowIdx];
// 					idBrittleFracture *window = static_cast<idBrittleFracture *>(gameLocal.entities[brittleIndex]);
// 		
// 					if (!gameLocal.entities[brittleIndex]->IsType(idBrittleFracture::Type))
// 						return;
// 					
// 					if (window->assignedRoom.IsValid())
// 					{
// 						if (window->assignedRoom.GetEntity()->entityNumber == locationEntity->entityNumber)
// 						{
// 							//Found a match. Assign the door to the window.							
// 							//gameRenderWorld->DebugArrow(colorGreen, gameLocal.entities[i]->GetPhysics()->GetOrigin(), window->GetPhysics()->GetOrigin(), 4, 120000); //Draw arrow from door to window.
// 							window->vacuumDoorsToClose.Append(i);
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
}

void idMeta::SetLifeboatsLeave()
{
	//TODO: optimize; don't iterate through every game entity.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (!gameLocal.entities[i]->IsType(idLifeboat::Type))
			continue;

		static_cast<idLifeboat *>(gameLocal.entities[i])->StartTakeoff();
	}
}

int idMeta::GetEnemiesRemaining(int includeSkullsavers)
{
	int enemiesRemaining = 0;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden() || entity->health <= 0)		//sanity check
			continue;

		if (!entity->IsType(idAI::Type))
			continue;

		if (static_cast<idActor*>(entity)->team != TEAM_ENEMY)
			continue;

		if (entity->petNode.Owner() != NULL) //is it a pet
			continue;

		enemiesRemaining++;
	}

	if (includeSkullsavers)
	{
		for (idEntity* skullsaverEnt = gameLocal.skullsaverEntities.Next(); skullsaverEnt != NULL; skullsaverEnt = skullsaverEnt->skullsaverNode.Next())
		{
			if (!skullsaverEnt)
				continue;

			if (skullsaverEnt->IsType(idSkullsaver::Type))
			{
				enemiesRemaining++;
			}
		}
	}

	return enemiesRemaining;
}

struct cameraPair_t {
	idCameraSplice* spliceEnt;
	idSecurityCamera* secCamEnt;
	float dist;
};

static int cmpCameraPairs(cameraPair_t* a, cameraPair_t* b)
{
	return a->dist - b->dist;
}

void idMeta::PopulateCameraSplices()
{
	//First, get total amount of cameras in level.
	int amountOfCameras = gameLocal.securitycameraEntities.Num();
	this->totalSecuritycamerasAtGameStart = amountOfCameras;

	//Now get the total amount of camerasplices.
	idList<int> spliceEntNums;
	int amountOfSplices = 0;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent || !ent->IsType(idCameraSplice::Type))
			continue;

		amountOfSplices++;
		spliceEntNums.Append(ent->entityNumber);
	}

	if (amountOfCameras <= 0 && amountOfSplices <= 0)
		return; //Map has zero cameras & splices. Nothing more to be done, exit here.

	//Sanity check.
	if (amountOfCameras > 0 && amountOfSplices > 0 && amountOfCameras > amountOfSplices)
	{
		gameLocal.Error("Map has more securitycameras (%d) than camerasplices (%d). Map needs %d more camerasplices.", amountOfCameras, amountOfSplices, amountOfCameras - amountOfSplices);
		return;
	}

	//Splice count exceed camera count. Cull the splice entities down.
	if (amountOfSplices > amountOfCameras)
	{
		spliceEntNums.Shuffle(); //randomize the list

		int amountToDelete = amountOfSplices - amountOfCameras;
		for (int i = 0; i < amountToDelete; i++)
		{
			int entityNum = spliceEntNums[i];
			gameLocal.entities[entityNum]->PostEventMS(&EV_Remove, 0);
			spliceEntNums[i] = -1;
		}
	}

	// Generate pairs from each camera to each splice
	idList<cameraPair_t> pairs;
	cameraPair_t pair;
	int indexCounter = 0;

	for (idEntity* ent = gameLocal.securitycameraEntities.Next(); ent != NULL; ent = ent->securitycameraNode.Next())
	{
		pair.secCamEnt = static_cast<idSecurityCamera*>(ent);
		pair.secCamEnt->cameraIndex = indexCounter; //While we're here.... assign index numbers to all the cameras.
		indexCounter++;
		for (int i = 0; i < spliceEntNums.Num(); ++i)
		{
			if (spliceEntNums[i] != -1)
			{
				pair.spliceEnt = static_cast<idCameraSplice*>(gameLocal.entities[spliceEntNums[i]]);
				pair.dist = (pair.secCamEnt->GetPhysics()->GetOrigin() - pair.spliceEnt->GetPhysics()->GetOrigin()).Length();
				pairs.Append(pair);
			}
		}
	}

	// Sort the pairs by distance
	pairs.Sort((idList<cameraPair_t>::cmp_t *)&cmpCameraPairs);

	// Now assign based on this ordering
	idList<idCameraSplice*> assignedSplices;
	idList<idSecurityCamera*> assignedCameras;
	for (int i = 0; i < pairs.Num(); ++i)
	{
		cameraPair_t& temp = pairs[i];
		if (assignedSplices.FindIndex(temp.spliceEnt) == -1 &&
			assignedCameras.FindIndex(temp.secCamEnt) == -1)
		{
			assignedSplices.Append(temp.spliceEnt);
			assignedCameras.Append(temp.secCamEnt);
			temp.spliceEnt->AssignCamera(temp.secCamEnt);
		}
	}
}

void idMeta::SetPlayerExitedCryopod(bool value)
{
	cryoExitTimer = gameLocal.time + AI_AVOID_CRYOEXIT_ROOM_TIME;
	playerHasExitedCryopod = value;
}

bool idMeta::GetPlayerExitedCryopod()
{
	return playerHasExitedCryopod;
}



int idMeta::GetCryoexitLocationEntnum()
{
	return cryoExitLocationEntnum;
}

int idMeta::GetCryoExitTime()
{
	return cryoExitTimer;
}

bool idMeta::SpawnEnemies(int cryoLocationEntnum, bool doCryospawnCheck)
{
	int amountOfEnemiesToSpawn = gameLocal.world->spawnArgs.GetInt("enemies_max", "5");

	if (amountOfEnemiesToSpawn <= 0)
	{
		gameLocal.Printf("idMeta: map 'enemies_max' is set to 0. Skipping enemy spawn.\n");
		return true;
	}

	idList<int> spawnCandidates;
	spawnSpot_t		spot;
	spot.ent = gameLocal.FindEntityUsingDef(NULL, "info_enemyspawnpoint");

	if (!spot.ent)
	{
		gameLocal.Printf("idMeta: no info_enemyspawnpoint entities found. Skipping enemy spawn.\n");
		return true;
	}



	int spawnpointsInExitroom = 0;
	int spawnpointsInAirless = 0;
	int totalspawnpoints = 0;

	while (spot.ent)
	{
		totalspawnpoints++;

		if (gameLocal.GetAirlessAtPoint(spot.ent->GetPhysics()->GetOrigin() + idVec3(0,0,1))) //Check if there's oxygen at this respawn pad.
		{
			//No oxygen at this spot. Skip it, so that we're not spawning someone inside zero-g.
			gameLocal.Warning("idMeta: info_enemyspawnpoint '%s' is in airless room.", spot.ent->GetName());
			spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint");
			spawnpointsInAirless++;
			continue;
		}


		//Check if the cryospawn exit is in this room. If so, skip it.
		
		if (cryoLocationEntnum >= 0 && doCryospawnCheck)
		{
			idLocationEntity *locationEnt = gameLocal.LocationForEntity(spot.ent);
			if (locationEnt)
			{
				if (locationEnt->entityNumber == cryoLocationEntnum)
				{
					//Spawnpoint is in the SAME ROOM as the cryo exit. Do not spawn here.
					gameLocal.Printf("idMeta: skipping enemyspawnpoint in cryospawn exit room.\n");
					spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint");
					spawnpointsInExitroom++;
					continue;
				}
			}
		}


		spawnCandidates.Append(spot.ent->entityNumber);

		//iterate to next spot.
		spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint");
	}

	if (spawnCandidates.Num() <= 0)
	{
		gameLocal.Warning("idMeta: map has no viable info_enemyspawnpoint entities. Skipping enemy spawn.");
		return false;
	}

	
	if (amountOfEnemiesToSpawn > spawnCandidates.Num())
	{
		gameLocal.Warning("SpawnEnemies(): trying to spawn %d enemies but map has insufficient info_enemyspawnpoints.\n\nViable spawnpoints: %d (out of %d total spawnpoints)\n * skipped %d spawnpoints because they were in cryospawn exit room\n * skipped %d spawnpoints because they were in airless rooms\n", amountOfEnemiesToSpawn, spawnCandidates.Num(), totalspawnpoints, spawnpointsInExitroom, spawnpointsInAirless);
		return false;
	}

	//Randomize the list of spawnpoints. This is so that if we have a surplus, we use the full spectrum of spawnpoints instead of just the first XX of them.
	spawnCandidates.Shuffle();


	//Get list of enemy types.
	idStrList enemytypeList;
	
	idStr rawloadout;

	if (g_spawnfilter.GetString()[0] != '\0')
	{
		//Player is using a spawnfilter. Attempt to load a custom enemy_types list.
		idStr keyvalue = idStr::Format("enemy_types_%s", g_spawnfilter.GetString());
		rawloadout = gameLocal.world->spawnArgs.GetString(keyvalue.c_str());

		if (rawloadout[0] == '\0')
		{
			//empty. Fall back to the default values.
			rawloadout = gameLocal.world->spawnArgs.GetString("enemy_types");
		}
	}
	else
	{
		//Player is not using a spawnfilter.
		rawloadout = gameLocal.world->spawnArgs.GetString("enemy_types");
	}


	rawloadout.StripTrailingWhitespace();
	rawloadout.StripLeading(' ');
	for (int j = rawloadout.Length(); j >= 0; j--)
	{
		if (rawloadout[j] == ',')
		{
			enemytypeList.Append(rawloadout.Mid(j + 1, rawloadout.Length()).c_str());
			rawloadout = rawloadout.Left(j); //Truncate the string and continue.
		}
		else if (j <= 0)
		{
			enemytypeList.Append(rawloadout.Mid(0, rawloadout.Length()).c_str());
		}
	}		

	if (enemytypeList.Num() <= 0)
	{
		gameLocal.Error("idMeta: 'enemy_types' is empty. Skipping enemy spawn.\n");
		return true;
	}


	//Name generator.	
	#define NAMEINDEX_START 300
	#define NAMEINDEX_END   320
	idStrList nameList;
	for (int i = NAMEINDEX_START; i < NAMEINDEX_END; i++)
	{
		const char* keyname = va("#str_piratename_%05d", i); //idStr::Format acts weird here
		const char *keyvalue = common->GetLanguageDict()->GetString(keyname);

		if (keyvalue[0] != '\0')
		{
			nameList.Append(keyvalue);
		}
	}


	for (int i = 0; i < amountOfEnemiesToSpawn; i++)
	{
		//Set up position.
		idVec3 spawnPosition;
		spawnPosition = gameLocal.entities[spawnCandidates[i]]->GetPhysics()->GetOrigin() + idVec3(0,0,1);

		//Randomly select what kind of bad guy to spawn.
		int enemytypeIndex = gameLocal.random.RandomInt(enemytypeList.Num());
		const char* enemytypeName = enemytypeList[enemytypeIndex].c_str();

		//Set the angle.
		float yaw = gameLocal.entities[spawnCandidates[i]]->GetPhysics()->GetAxis().ToAngles().yaw;

		//Randomly select name.
		int nameIndex = gameLocal.random.RandomInt(nameList.Num());
		idStr actorName = nameList[nameIndex];
		nameList.RemoveIndex(nameIndex);

		idEntity *baddie;
		idDict args;
		args.Set("classname", enemytypeName);
		args.SetVector("origin", spawnPosition);
		args.SetFloat("angle", yaw);
		args.Set("displayname", actorName.c_str());
		gameLocal.SpawnEntityDef(args, &baddie);
	}

	gameLocal.Printf("idMeta: spawned %d enemies.\n", amountOfEnemiesToSpawn);
	return true;
}

void idMeta::SetWalkietalkieLure(idEntity *ent)
{
	if (ent == NULL)
		return;

	walkietalkieLureTask = ent;
	beamLureTarget->SetOrigin(ent->GetPhysics()->GetOrigin());
	beamLure->Show();	

	beamRectangle->SetOrigin(ent->GetPhysics()->GetOrigin() + idVec3(0,0,40));
	beamRectangle->Show();
}

void idMeta::DestroyNearbyGlassPieces(idVec3 position)
{
	if (glassdestroyTimer > gameLocal.time)
		return;

	glassdestroyTimer = gameLocal.time + 300; //cooldown between destroy-nearby-glass calls.

	idEntity		*entityList[MAX_GENTITIES];
	int entityCount = gameLocal.EntitiesWithinRadius(position, CLEANUP_RADIUS, entityList, MAX_GENTITIES);
	for (int i = 0; i < entityCount; i++)
	{
		idEntity *ent = entityList[i];

		if (!ent)
			continue;

		if (ent->IsHidden() || !ent->IsType(idGlassPiece::Type) || ent == this)
			continue;

		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() == ent)
			{
				continue;
			}
		}

		static_cast<idGlassPiece *>(ent)->ShatterAndRemove();
	}
}


int idMeta::GetCombatStartTime()
{
	//GUI timer for the combat state.
	return combatstateStartTime;
}

void idMeta::StartFTLRescueSequence()
{
	if (!gameLocal.GetLocalPlayer()->isInOuterSpace())
		return;
	
	

	if (!gameLocal.GetLocalPlayer()->noclip)
	{
		//Inflict damage.
		if (!gameLocal.GetLocalPlayer()->inDownedState)
		{
			gameLocal.GetLocalPlayer()->Damage(NULL, NULL, vec3_zero, "damage_outsideftl", 1.0f, 0);
		}

		idEntity *airlockEnt = GetEmptyAirlock();
		if (airlockEnt != NULL)
		{
			//We found a suitable airlock to teleport player to.
			gameLocal.GetLocalPlayer()->playerView.Flash(colorMagenta, 1000);

			//Do a fov lerp effect.
			#define AIRLOCK_FOV_AMOUNT -80
			#define AIRLOCK_FOV_LERPTIME 1500
			gameLocal.GetLocalPlayer()->Event_SetFOVLerp(AIRLOCK_FOV_AMOUNT, 0); //snap to a very wide lens
			gameLocal.GetLocalPlayer()->Event_SetFOVLerp(0, AIRLOCK_FOV_LERPTIME); //then lerp back to normal fov

			idVec3 teleportPos = airlockEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, 3); //Raise it up a tad just to make sure player's feet don't get clipped into floor.
			idAngles teleportAngle = airlockEnt->GetPhysics()->GetAxis().ToAngles();
			teleportAngle.yaw += 90;
			gameLocal.GetLocalPlayer()->Teleport(teleportPos, teleportAngle, airlockEnt); //teleport player.

			airlockEnt->StartSound("snd_vo_hyperrescue", SND_CHANNEL_VOICE);

			//("HYPERSPACE EMERGENCY RESCUE ACTIVATED");
		}

		if (!gameLocal.GetLocalPlayer()->airless && !gameLocal.GetLocalPlayer()->inDownedState)
		{
			gameLocal.GetLocalPlayer()->SetFallState(true, false); //make player lie down, if in a pressurized room.
		}
	}
}

idEntity* idMeta::GetEmptyAirlock()
{
	//Get closest, nearest airlock.
	idEntity *closestAirlock = NULL;
	float closestDistance = 99999;

	for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
	{
		if (!airlockEnt)
			continue;

		if (!airlockEnt->IsType(idAirlock::Type))
			continue;

		//do a trace check.
		idBounds bound = idBounds(idVec3(-40, -40, 3), idVec3(40, 40, 96));
		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, airlockEnt->GetPhysics()->GetOrigin(), airlockEnt->GetPhysics()->GetOrigin(), bound, MASK_SOLID | MASK_SHOT_BOUNDINGBOX, NULL);

		if (boundTr.fraction < 1)
			continue; //Something is blocking this airlock. Skip this one.

		float distance = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - airlockEnt->GetPhysics()->GetOrigin()).Length();
		if (distance < closestDistance)
		{
			closestDistance = distance;
			closestAirlock = airlockEnt;
		}
	}

	if (closestAirlock != NULL)
	{
		return closestAirlock;
	}

	return NULL;
}

//============================= FTL EVENTS =============================

//When the sabotage shutdown lever is used.
void idMeta::FTL_DoShutdown()
{
	//Shut down the FTL drive.
	if (GetFTLDrive.IsValid())
	{
		static_cast<idFTL *>(GetFTLDrive.GetEntity())->StartShutdown();
	}
}

//When the FTL drive enters a shutdown state.
void idMeta::FTL_JumpEnded()
{
	//the FTL Jump has just ended.
	SetAll_FTL_Signage(FTLSIGN_OFF);

	//unlock all airlock purge gates.
	UnlockAirlockLockdown();

	//Disable all the sabotage shutdown entities.
	EnableSabotageLevers(false);

	//Blast all the electricalboxes on the ship.
	EnableAllElectricalboxes(false);
}

//When the FTL begins its reboot countdown.
void idMeta::FTL_CountdownStarted()
{
	SetAllInfostationsMode(INFOS_FTLCOUNTDOWN); //make info stations show the ftl countdown.
	SetAll_FTL_Signage(FTLSIGN_COUNTDOWN); //display the ftl countdown on doorframes.	
}

//When the FTL drive finishes rebooting and is now active.
void idMeta::FTL_JumpStarted()
{
	SetAll_FTL_Signage(FTLSIGN_ACTIVE);
    EnableSabotageLevers(true);
    EnableAllElectricalboxes(true);
}


bool idMeta::FTL_IsActive()
{
	if (!GetFTLDrive.IsValid())
		return false;

	return static_cast<idFTL *>(GetFTLDrive.GetEntity())->IsJumpActive(false, false);
}



void idMeta::EnableAllElectricalboxes(bool value)
{
    for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
    {
        if (!ent)
            continue;

        if (ent->IsType(idElectricalBox::Type))
        {
            if (value)
            {
                ent->DoRepairTick(1); //repair.
            }
            else
            {
                ent->Damage(this, this, vec3_zero, "damage_suicide", 1.0f, 0); //blow up.
            }
        }
    }
}

void idMeta::EnableSabotageLevers(bool value)
{
    for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
    {
        if (!ent)
            continue;

        if (ent->IsType(idSabotageShutdown::Type))
        {
            static_cast<idSabotageShutdown *>(ent)->SetEnabled(value);
        }
    }
}

void idMeta::OnEnemySeeHostile()
{
	//When an enemy AI sees someone they don't like.
	combatstateStartTime = gameLocal.time;
}

void idMeta::NameTheEnemies()
{
	//Give random names to the enemies.

	//localized list of names.
	#define NAMEINDEX_START 300
	#define NAMEINDEX_END   320
	idStrList nameList;
	for (int i = NAMEINDEX_START; i < NAMEINDEX_END; i++)
	{
		const char* keyname = va("#str_piratename_%05d", i); //idStr::Format acts weird here
		const char *keyvalue = common->GetLanguageDict()->GetString(keyname);

		if (keyvalue[0] != '\0')
		{
			nameList.Append(keyvalue);
		}
	}


	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsType(idActor::Type) || entity->team != TEAM_ENEMY)
			continue;

		if (entity->displayName.Length() > 0)
			continue;

		int nameIndex = gameLocal.random.RandomInt(nameList.Num());
		idStr actorName = nameList[nameIndex];
		nameList.RemoveIndex(nameIndex);

		entity->displayName = actorName;
		entity->spawnArgs.Set("displayname", actorName);
	}	
}

//May be deprecated. Is not called right now...
void idMeta::UpdateBombStartCheck()
{
	if (enemydestroyedCheckState != ENEMYDESTROYEDSTATECHECK_CATS_AWAITING_RESCUE)
		return;

	//if (!gameLocal.world->spawnArgs.GetBool("meta_endcheck", "1") || enemydestroyedCheckState != ENEMYDESTROYEDSTATECHECK_WAITDELAY || gameLocal.time < enemydestroyedCheckTimer || metaState != METASTATE_ACTIVE)
	if (!gameLocal.world->spawnArgs.GetBool("meta_endcheck", "1") || gameLocal.time < enemydestroyedCheckTimer || metaState != METASTATE_ACTIVE)
		return;

	//int enemyBodiesRemaining = 0;
	//for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	//{
	//	if (!entity)
	//		continue;		
	//
	//	if (!entity->IsActive() || !entity->IsType(idAI::Type) || entity->team != TEAM_ENEMY || entity->petNode.Owner() != NULL)
	//		continue;		
	//
	//	enemyBodiesRemaining++;
	//}
	//
	//if (enemyBodiesRemaining > 0)
	//	return;

	int openedCages = 0;
	int totalcages = 0;
	for (idEntity* cageEnt = gameLocal.catcageEntities.Next(); cageEnt != NULL; cageEnt = cageEnt->catcageNode.Next())
	{
		if (!cageEnt)
			continue;

		if (!cageEnt->IsType(idCatcage::Type))
			continue;

		totalcages++;

		if (static_cast<idCatcage *>(cageEnt)->IsOpenedCompletely())
		{
			openedCages++;
		}
	}

	if (totalcages > openedCages || totalcages <= 0)
		return;




	//Ok, all enemy cat cages are unlocked.
	//enemydestroyedCheckState = ENEMYDESTROYEDSTATECHECK_CATS_AWAITING_RESCUE_DELAY;
	//enemydestroyedCheckTimer = gameLocal.time + 120000; //delay for baddies to spawn in.


		

    //if (gameLocal.world->spawnArgs.GetBool("bombtest", "0"))
    //{
    //    //So: start the bomb sequence.
    //    enemydestroyedCheckState = ENEMYDESTROYEDSTATECHECK_NONE;
    //    gameLocal.RunMapScript("_startBombSequence");
    //}
    //else
    //{
    //    StartPostGame();
    //}
	
}

void idMeta::Event_GetCombatState(void)
{
	idThread::ReturnInt(GetCombatState());
}

//Script call.
void idMeta::Event_SetCombatState(int value)
{
    if (value >= 2)
    {
        AlertAIFriends(NULL);
    }
    //else if (value == 1)
    //{
    //    GotoCombatSearchState();
    //}
    else
    {
        GotoCombatIdleState();
    }
}

//script call.
void idMeta::Event_GetAliveEnemies()
{
	int count = 0;
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsActive() || entity->IsHidden() || entity == this || entity->team != TEAM_ENEMY || !entity->IsType(idActor::Type))
			continue;

		count++;
	}

	idThread::ReturnInt(count);
}

// Applies a certain stun state to all actors in the map. Used for (e.g.) big events that stagger/knockdown everyone on board.
void idMeta::Event_StartGlobalStunState(const char* aiDamageDef)
{
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->IsActive() || entity->IsHidden() || entity == this || entity->team != TEAM_ENEMY || !entity->IsType(idActor::Type))
			continue;

		static_cast<idActor*>(entity)->StartStunState(aiDamageDef);
	}
}

bool idMeta::IsEntInVentOrAirlock(idEntity *ent)
{
	idVec3 entPosition = ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
	idVec3 lkpPosition = lkpEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);

	if (gameLocal.GetPositionIsInsideConfinedspace(entPosition))
	{
		return true;
	}

	//Check if LKP is inside an airlock. If so, start airlock purge.
	for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
	{
		if (!airlockEnt)
			continue;

		if (!airlockEnt->IsType(idAirlock::Type))
			continue;
		
		if (airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(entPosition))
		{
			return true;
		}

		if (airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(lkpPosition) && !lkpEnt->IsHidden())
		{
			return true;
		}
	}

	if (gameLocal.GetPositionIsInsideConfinedspace(lkpPosition) && !lkpEnt->IsHidden())
	{
		return true;
	}
	

	return false;
}

void idMeta::SetWorldBaffled(bool value)
{
	isWorldBaffled = value;
}

bool idMeta::GetWorldBaffled()
{
	return isWorldBaffled;
}

void idMeta::StartReinforcementsSequence()
{
	if (reinforcementPhase != REINF_NONE)
		return;

	//We no longer have this delay before the pirate ship spawns.
	//reinforcementPhase = REINF_SPAWNDELAY;
	//reinforcementPhaseTimer = gameLocal.time + REINFORCEMENTS_SPAWNDELAY;

	
	reinforcementPhase = REINF_PIRATES_ENROUTE;
	SpawnPirateShip();
}

bool idMeta::SpawnPirateShip()
{
	//Iterate through all pirate ships.
	int pirateShipsFound = 0;

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idPirateship::Type))
		{
			if (!static_cast<idPirateship *>(ent)->IsDormant())
				continue;

			//start this pirateship's entrance.
			static_cast<idPirateship *>(ent)->StartEntranceSequence();
			pirateShipsFound = true;
		}
	}

	if (gameLocal.world->spawnArgs.GetBool("objectives", "1"))
	{
		gameLocal.GetLocalPlayer()->SetObjectiveText("", false);
	}

	if (pirateShipsFound <= 0)
	{
		gameLocal.Warning("No func_pirateship found.\n");
		return false;
	}

	gameLocal.RunMapScript("_startReinforcements");

	if (gameLocal.world->spawnArgs.GetBool("objectives", "1"))
	{
		gameLocal.GetLocalPlayer()->SetObjectiveText("#str_obj_boardingship", true, "icon_obj_pirates");
	}

	return true;
}

void idMeta::SetReinforcementPiratesAllSpawned()
{
	reinforcementPhase = REINF_PIRATES_ALL_SPAWNED;
}

void idMeta::UpdateCagecageObjectiveText()
{
	int totalcages = 0;
	int openedCages = 0;
	for (idEntity* cageEnt = gameLocal.catcageEntities.Next(); cageEnt != NULL; cageEnt = cageEnt->catcageNode.Next())
	{
		if (!cageEnt)
			continue;

		if (!cageEnt->IsType(idCatcage::Type))
			continue;

		totalcages++;

		if (static_cast<idCatcage *>(cageEnt)->IsOpened())
		{
			openedCages++;
		}
	}

	if (gameLocal.world->spawnArgs.GetBool("objectives", "1") && (openedCages < totalcages))
	{
		gameLocal.GetLocalPlayer()->SetObjectiveText(idStr::Format2(common->GetLanguageDict()->GetString("#str_obj_releasecatcrew"), openedCages, totalcages).c_str(), true, "icon_obj_releasecatcrew");
	}
}

void idMeta::SetCombatDurationTime(int value)
{
	combatstateDurationTime = value;
}

//Control the lockdown on trash chutes
void idMeta::SetEnableTrashchutes(bool value)
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idTrashchute::Type))
		{
			static_cast<idTrashchute *>(ent)->Event_ChuteEnable(value ? 1 : 0);
		}

		if (ent->IsType(idTrashExit::Type))
		{
			static_cast<idTrashExit *>(ent)->Event_ChuteEnable(value ? 1 : 0);
		}		
	}
}

void idMeta::SetEnableAirlocks(bool value)
{
	for (idEntity* ent = gameLocal.airlockEntities.Next(); ent != NULL; ent = ent->airlockNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idAirlock::Type))
		{
			static_cast<idAirlock*>(ent)->SetFuseboxGateOpen(value);
		}
	}
}

void idMeta::SetEnableVents(bool value)
{
	//gameLocal.GetLocalPlayer()->SetROQVideoState(1);

	for (idEntity* ent = gameLocal.ventdoorEntities.Next(); ent != NULL; ent = ent->ventdoorNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idVentdoor::Type))
		{
			static_cast<idVentdoor*>(ent)->Event_SetBarricade(value ? 0 : 1);
		}
	}
}



void idMeta::SetEnableWindows(bool value)
{
	for (idEntity* ent = gameLocal.windowshutterEntities.Next(); ent != NULL; ent = ent->windowshutterNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idWindowShutter::Type))
		{
			static_cast<idWindowShutter*>(ent)->SetShutterOpen(value);
		}
	}
}

//Call this to UNLOCK all doors. This is for when reinforcements happen.
void idMeta::SetEnableDoorBarricades()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden() || !ent->IsType(idDoor::Type) || ent->IsType(idVentdoor::Type))
			continue;

		if (!static_cast<idDoor *>(ent)->IsBarricaded())
			continue;

		static_cast<idDoor *>(ent)->RemoveBarricade();
	}
}

//Returns whether a fusebox system is locked or not.
bool idMeta::IsFuseboxLocked(int systemIndex)
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idMaintPanel::Type))
		{
			if (static_cast<idMaintPanel*>(ent)->IsDone())
			{
				continue; //panel is already done/unlocked. skip it.
			}

			if (static_cast<idMaintPanel*>(ent)->GetSystemIndex() == systemIndex)
				return true;
		}
	}

	return false;
}

idEntity * idMeta::GetSecurityCameraViaIndex(int index)
{
	for (idEntity* secCamEnt = gameLocal.securitycameraEntities.Next(); secCamEnt != NULL; secCamEnt = secCamEnt->securitycameraNode.Next())
	{
		if (secCamEnt == NULL)
			continue;

		if (!secCamEnt->IsType(idSecurityCamera::Type))
			continue;

		if (static_cast<idSecurityCamera *>(secCamEnt)->cameraIndex == index)
			return secCamEnt;
	}

	return NULL;
}

bool idMeta::DoHighlighter(idEntity *ent1, idEntity *ent2)
{
	if (highlighterEnt == NULL)
		return false;

	return highlighterEnt->DoHighlight(ent1, ent2);
}

// SW 26th March 2025: chiefly for testing, but maybe someone will get some use out of it?
void idMeta::Event_DoHighlighter(idEntity* e1, idEntity* e2)
{
	DoHighlighter(e1, e2);
}

bool idMeta::GetHightlighterActive()
{
	if (highlighterEnt == NULL)
		return false;

	return highlighterEnt->IsHighlightActive();
}

void idMeta::DoHightlighterThink()
{
	if (highlighterEnt == NULL)
		return;	

	highlighterEnt->Think();
}

void idMeta::DrawHighlighterBars()
{
	if (highlighterEnt == NULL)
		return;

	highlighterEnt->DrawBars();
}

void idMeta::SkipHighlighter()
{
	if (highlighterEnt == NULL)
		return;

	return highlighterEnt->DoSkip();
}

// SW 31st March 2025
void idMeta::Event_SetDebrisBurst(const char* defName, idVec3 position, int count, float radius, float speed, idVec3 direction)
{
	gameLocal.SetDebrisBurst(defName, position, count, radius, speed, direction);
}

void idMeta::Event_LaunchScriptedProjectile(idEntity* owner, char* damageDef, const idVec3 &spawnPos, const idVec3 &spawnTrajectory)
{
	idThread::ReturnEntity(gameLocal.LaunchProjectile(owner, damageDef, spawnPos, spawnTrajectory));
}

int idMeta::GetTotalCatCages()
{
	return totalCatCagesInLevel;
}

int idMeta::CalculateTotalCatCages()
{
	int totalcages = 0;
	for (idEntity* cageEnt = gameLocal.catcageEntities.Next(); cageEnt != NULL; cageEnt = cageEnt->catcageNode.Next())
	{
		if (!cageEnt)
			continue;

		if (!cageEnt->IsType(idCatcage::Type))
			continue;

		totalcages++;
	}

	return totalcages;
}

//Make player enter cat pod.
void idMeta::SetPlayerEnterCatpod()
{
	if (!catpodInterior.IsValid())
	{
		gameLocal.Error("SetPlayerEnterCatpod: cannot find env_catpod_interior.\n");
		return;
	}

	//Enter the catpod.
	if (!catpodInterior.GetEntity()->IsType(idCatpodInterior::Type))
	{
		gameLocal.Error("SetPlayerEnterCatpod: catpodinterior entity is wrong entity type.\n");
		return;
	}

	static_cast<idCatpodInterior*>(catpodInterior.GetEntity())->SetPlayerEnter();
	playerHasEnteredCatpod = true;
}

bool idMeta::GetPlayerHasEnteredCatpod()
{
	return  playerHasEnteredCatpod;
}

bool idMeta::GetPlayerIsCurrentlyInCatpod()
{
	if (!catpodInterior.IsValid())
	{
		return false;
	}

	if (!catpodInterior.GetEntity()->IsType(idCatpodInterior::Type))
	{
		return false;
	}

	return static_cast<idCatpodInterior*>(catpodInterior.GetEntity())->GetPlayerIsInPod();
}

void idMeta::GenerateKeypadcodes()
{
	//first, check whether there are any keypads in the level.
	bool levelHasKeypads = false;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idKeypad::Type))
		{
			levelHasKeypads = true;
			break;
		}
	}

	//If level does not have any keypads at all, then do not do the keycode generation stuff. Exit here.
	if (!levelHasKeypads)
		return;

	//Generate a full list of all possible keycodes, from the keycodes.txt file.
	void*		buffer;
	idStr		rawDictionary;
	int			i;
	idStrList	keys;

	#define KEYCODEFILENAME "keycodes.txt"
	if (fileSystem->ReadFile(KEYCODEFILENAME, &buffer) > 0)
	{
		rawDictionary = (char*)buffer;
		fileSystem->FreeFile(buffer);
	}
	else
	{
		gameLocal.Error("failed to load %s", KEYCODEFILENAME);
		return;
	}

	rawDictionary.StripTrailingWhitespace(); //remove whitespace.
	rawDictionary.StripLeading(' ');		//remove whitespace.
	rawDictionary.ToUpper();

	keys.Clear();
	for (i = rawDictionary.Length(); i >= 0; i--)
	{
		if (rawDictionary[i] == '\n' || rawDictionary[i] == '\r')
		{
			if (rawDictionary.Mid(i + 1, rawDictionary.Length()).IsEmpty())
			{
				rawDictionary = rawDictionary.Left(i);
				continue;
			}

			keys.AddUnique(rawDictionary.Mid(i + 1, rawDictionary.Length()));

			rawDictionary = rawDictionary.Left(i); //strip.
		}

		if (i <= 0)
		{
			keys.AddUnique(rawDictionary.Mid(0, rawDictionary.Length()));
		}
	}

	
	//we now have a list ("idStrList keys") of all possible codes.
	
	//Now, figure out how many codes we need to generate.

	//Gather all the fuseboxes.
	idList<int> allSystemIndexes;	
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idMaintPanel::Type))
			continue;

		int systemIndex = ent->spawnArgs.GetInt("systemindex");
		if (allSystemIndexes.Find(systemIndex))
			continue;

		allSystemIndexes.Append(systemIndex);
	}

	//We now know how many types of fuseboxes we have in map.
	//We need to generate this many keycodes.
	int numberOfSystemIndexes = allSystemIndexes.Num();
	
	//Generate codes.	
	idStrList selectedCodes = PickRandomCodes(numberOfSystemIndexes, keys);	
	
	//Now assign the codes to the keypads.
	AssignCodesToKeypads(selectedCodes, allSystemIndexes);

	//Keypads now all have codes assigned to them.
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		//iterate through all notes/tablets/etc.

		if (!ent)
			continue;		

		if (!ent->IsType(idNoteWall::Type) && !ent->IsType(idTablet::Type))
			continue;

		//Determine if note has password delimiter.
		idStr guiText = ent->spawnArgs.GetString("gui_parm0");
		int hasStringDelimiter = guiText.Find("%s", false);

		if (ent->targets.Num() <= 0)
		{
			if (hasStringDelimiter >= 0)
			{
				gameLocal.Error("note '%s' has string percent-operator but does not target a fusebox. (%s)", ent->GetName(), guiText.c_str());
				return;
			}

			continue;
		}

		//Get the associated code with this note.
		idStr code = ent->targets[0].GetEntity()->spawnArgs.GetString("code");
		if (code.Length() <= 0)
		{
			if (ent->targets[0].GetEntity()->IsType(idKeypad::Type))
				gameLocal.Error("keypad '%s' has no code to assign to note.", ent->targets[0].GetEntity()->GetName());
			else
				gameLocal.Error("Note '%s' needs to target a keypad (it currently targets '%s')", ent->GetName(), ent->targets[0].GetEntity()->GetName());

			return;
		}

		if (code.Length() <= 3)
		{
			gameLocal.Error("keypad '%s' code is too short? ('%s')", ent->targets[0].GetEntity()->GetName(), code.c_str());
			return;
		}

		idStr firstletter = code.Mid(0, 1);
		idStr secondletter = code.Mid(1, 1);
		idStr thirdletter = code.Mid(2, 1);
		idStr fourthletter = code.Mid(3, 1);

		idStr outputText;
		int codeIndexes = ent->spawnArgs.GetInt("codeindexes", "1234");
		if (codeIndexes == 1234)
		{
			//Default. 1234.
			outputText = idStr::Format(guiText, firstletter.c_str(), secondletter.c_str(), thirdletter.c_str(), fourthletter.c_str());
		}
		else if (codeIndexes == 12)
		{
			//12.
			outputText = idStr::Format(guiText, firstletter.c_str(), secondletter.c_str());
		}
		else if (codeIndexes == 34)
		{
			//34.
			outputText = idStr::Format(guiText, thirdletter.c_str(), fourthletter.c_str());
		}
		else if (codeIndexes == 4321)
		{
			outputText = idStr::Format(guiText, fourthletter.c_str(), thirdletter.c_str(), secondletter.c_str(), firstletter.c_str());
		}
		else
		{
			//Nothing matched....
			gameLocal.Error("idMeta::GenerateKeypadcodes couldn't parse keypad '%s' codeindexes value '%d'\n", ent->GetName(), codeIndexes);
			return;
		}
		
		ent->Event_SetGuiParm("gui_parm0", outputText); //assign text to the note.

		if (ent->spawnArgs.GetBool("give_parm2_name", "0"))
		{
			//This is probably a forget-me-not note. Give its parm2 the name of the fusebox it's associated with.

			idEntity* keypadEnt = ent->targets[0].GetEntity();
			if (keypadEnt != nullptr)
			{
				//Now find what the keypad targets.
				if (keypadEnt->targets.Num() > 0)
				{
					//We now have the fusebox associated with this note.
					idStr systemName = "";

					idEntity* fuseboxEnt = keypadEnt->targets[0].GetEntity();
					int systemIndex = fuseboxEnt->spawnArgs.GetInt("systemindex", "-1");
					
					if (systemIndex == SYS_TRASHCHUTES)
						systemName = "#str_gui_armstatsmenu_100328"; //trash fusebox
					else if (systemIndex == SYS_AIRLOCKS)
						systemName = "#str_gui_armstatsmenu_100327"; //airlock fusebox
					else if (systemIndex == SYS_VENTS)
						systemName = "#str_gui_armstatsmenu_100325"; //vent fusebox
					else if (systemIndex == SYS_WINDOWS)
						systemName = "#str_gui_armstatsmenu_100326"; //windows fusebox
					else
					{
						common->Warning("Note '%s' has invalid fusebox systemindex (%d)\n", ent->GetName(), systemIndex);
						systemName = "???";
					}

					ent->Event_SetGuiParm("gui_parm2", systemName.c_str()); //assign text to the note.
				}
			}
		}
	}
}

//Assign code to keypad. We ensure that the SAME CODE is applied to keypads that are on the SAME SYSTEMINDEX.
//Example: all trashchutes must use the same code; we do NOT want different trashchutes with different codes.
void idMeta::AssignCodesToKeypads(idStrList selectedCodes, idList<int> allSystemIndexes)
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idKeypad::Type))
			continue;

		if (ent->targets.Num() <= 0)
		{
			gameLocal.Warning("AssignCodesToKeypads: keypad '%s' has no targets.", ent->GetName());
			continue;
		}

		//Found its associated fusebox.
		idEntity* fusebox = ent->targets[0].GetEntity();
		int systemIndex = fusebox->spawnArgs.GetInt("systemindex");
		
		//Find which index in the allsystemindexes this is.
		int codeIndex = -1;
		for (int i = 0; i < allSystemIndexes.Num(); i++)
		{
			if (allSystemIndexes[i] == systemIndex)
			{
				codeIndex = i;
				i = 99999; //break out.
			}
		}

		if (codeIndex < 0)
		{
			gameLocal.Error("AssignCodesToKeypads: bad systemindex value on '%s'", ent->GetName());
			return;
		}
		
		idStr newcode = selectedCodes[codeIndex];
		static_cast<idKeypad*>(ent)->SetCode(newcode);
	}
}

idStrList idMeta::PickRandomCodes(int amount, idStrList keys)
{
	idStrList results;

	for (int i = 0; i < amount; i++)
	{
		//Add a random code to the list.
		int randomIndex = gameLocal.random.RandomInt(keys.Num());
		idStr randomcode = keys[randomIndex];
		results.Append(randomcode);

		//Now, REMOVE all codes that start with its first letter.
		//By culling the list, this is how we ensure codes all start with a diffent letter.
		char firstLetter = randomcode[0];
		for (int k = keys.Num() - 1; k >= 0; k--)
		{
			if (keys[k].Find(firstLetter) == 0)
			{
				//matches the first letter. remove it from list.
				keys.RemoveIndex(k);
			}
		}
	}

	return results;
}

//Script call 'EnableRadiocheckin' calls this function.
void idMeta::SetEnableRadioCheckin(int value)
{
	if (radioCheckinEnt == NULL)
		return;
	
	//NOTE: this isn't working, as worldspawn meta_radiocheck mostly achieves the same effect.
	//radioCheckinEnt
}

//The mission milestone system. When player completes a special task of some kind in the mission.
void idMeta::SetMilestone(int milestoneIndex)
{
	if (milestoneIndex < 0 || milestoneIndex >= MAXMILESTONES)
	{
		//value out of range. We have 3 milestones (0,1,2), and the value we received is invalid. Exit.
		gameLocal.Warning("SetMilestone: index out of range ('%d')\n", milestoneIndex);
		return;
	}

	//Get level index of map we're currently in right now.
	int levelIndex = -1;
	idStr mapname;
	mapname = gameLocal.GetMapName();
	mapname.StripPath();
	mapname.StripFileExtension();
	mapname.ToLower();

	//Grab map def of current map.
	const idDecl* mapDecl = declManager->FindType(DECL_MAPDEF, mapname, false);
	const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(mapDecl);
	if (mapDef)
	{
		//found the map def.
		levelIndex = mapDef->dict.GetInt("levelindex");
	}

	if (levelIndex < 0)
	{
		gameLocal.Warning("SetMilestone: failed to find levelIndex for '%s'. Bad maps.def setup?\n", mapname.c_str());
		return;
	}

	//Flip the milestone true.
	idStr milestoneName = gameLocal.GetMilestonenameViaLevelIndex(levelIndex, milestoneIndex);
	gameLocal.persistentLevelInfo.SetBool(milestoneName.c_str(), true);
}

//Handle the stat that records how long the player is in outer space.
void idMeta::UpdatePlayerSpaceTimerStat()
{
	if (gameLocal.GetLocalPlayer()->airless)
	{
		//player is in outer space.

		if (!playerIsInSpace)
		{
			playerIsInSpace = true;
			playerSpaceTimerStart = gameLocal.time;
		}
	}
	else
	{
		//player is NOT in outer space.

		if (playerIsInSpace)
		{
			playerIsInSpace = false;

			int totalTimeToAddToStat = gameLocal.time - playerSpaceTimerStart;
			playerSpaceTimerTotal += totalTimeToAddToStat;
		}
	}
}

int idMeta::GetTotalTimeSpentInSpace()
{
	// SW 20th Feb 2025: If the player enters the postgame while in space,
	// there will be some outstanding amount of time that needs to be added to the total
	// before it is accurate.
	if (playerIsInSpace)
	{
		int totalTimeToAddToStat = gameLocal.time - playerSpaceTimerStart;
		playerSpaceTimerTotal += totalTimeToAddToStat;
	}

	return playerSpaceTimerTotal;
}

void idMeta::SetExitVR()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idVRVisor::Type))
		{
			static_cast<idVRVisor*>(ent)->SetExitVisor();
		}
	}
}

bool idMeta::GetEnemiesKnowOfNina()
{
	return enemiesKnowOfNina;
}

void idMeta::DoSignalkitDepletionCheck()
{
	if (gameLocal.time < signalkitDepletionTimer)
		return;

	signalkitDepletionTimer = gameLocal.time + SIGNALKIT_DEPLETION_CHECKINTERVAL;

	//We always want at least one signal kit in the level.
	//Check how many signal kits are in the level.

	idEntity* lostandfound = FindLostAndFoundMachine();	
	if (lostandfound == nullptr)
		return; //There is no lost and found in this level, so ignore.

	//Check if one is in lost and found.
	if (gameLocal.GetLocalPlayer()->IsEntityLostInSpace("moveable_item_signallamp"))
	{
		//There is a signal lamp in the lost and found. So, exit here.
		return;
	}

	int amountOfLampsInLevel = 0;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType( idMoveableItem::Type) || ent->health <= 0)
			continue;

		if (idStr::Icmp(ent->spawnArgs.GetString("classname"), "moveable_item_signallamp") != 0)
			continue;

		if (gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(ent))
		{
			//Player is carrying a signal lamp. Get outta here....
			return;
		}

		//Check if lost and found machine has clear LOS to the signal lamp.		
		if (lostandfound != nullptr)
		{
			idVec3 forward, up;
			lostandfound->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

			idVec3 machinePos = lostandfound->GetPhysics()->GetOrigin() + (forward * 16) + (up * 48);

			trace_t tr;
			gameLocal.clip.TracePoint(tr, machinePos, ent->GetPhysics()->GetOrigin(), MASK_SOLID, NULL);
			if (tr.fraction >= 1)
			{
				//Lost and found machine can clearly see a signal lamp near it. Get outta here...
				return;
			}
		}

		amountOfLampsInLevel++;
	}

	if (amountOfLampsInLevel <= 1)
	{		
		//spawn a signal lamp in outer space.
		idEntity* newLamp = NULL;
		idDict args;
		args.Clear();
		args.Set("classname", "moveable_item_signallamp");
		gameLocal.SpawnEntityDef(args, &newLamp);

		if (newLamp)
		{
			if (newLamp->IsType(idMoveableItem::Type))
			{
				static_cast<idMoveableItem*>(newLamp)->SetLostInSpace();
			}
		}		
	}
}

idEntity* idMeta::FindSpaceLocation()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idLocationEntity::Type))
			continue;

		if (idStr::Cmp(static_cast<idLocationEntity*>(ent)->GetLocation(), common->GetLanguageDict()->GetString("#str_00000")))
		{
			return ent;
		}
	}

	return NULL;
}

idEntity* idMeta::FindLostAndFoundMachine()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idLostAndFound::Type))
			continue;
		
		return ent;		
	}

	return NULL;
}

