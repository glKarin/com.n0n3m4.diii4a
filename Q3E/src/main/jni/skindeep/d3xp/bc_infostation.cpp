#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "WorldSpawn.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "idlib/LangDict.h"

#include "bc_ventdoor.h"
#include "bc_meta.h"
#include "bc_ftl.h"

#include "bc_maintpanel.h"
#include "bc_catcage.h"
#include "bc_infostation.h"

const int REBOOT_TIME = 5000;
const int LEGEND_UPDATETIME = 100;

const int MAX_LOCMARKERS = 10; //This is dependent on info_station.gui -- make sure the gui has enough loc* windowdefs to handle this quantity.



const int HACKBOX_FROBINDEX = 99;

#define BUTTON_INFOSTATIONS		1
#define BUTTON_HEALTHSTATIONS	2
#define BUTTON_OXYGENSTATIONS	3
#define BUTTON_ACCESSPANELS		4
#define BUTTON_TRASH			5
#define BUTTON_HUMAN			6
#define BUTTON_CAT				7
#define BUTTON_LOSTANDFOUND		8



#define BUTTON_MAP				100
#define BUTTON_FTL				101

#define BUTTON_REPAIRBOT        200

#define BUTTON_SAVEGAME         220


CLASS_DECLARATION(idStaticEntity, idInfoStation)
	//EVENT(EV_Activate, idInfoStation::Event_Activate)
	EVENT(EV_PostSpawn, idInfoStation::Event_PostSpawn)
END_CLASS

idInfoStation::idInfoStation(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	//make it repairable.
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);

	hackboxEnt = 0;
	lastSaveTime = 0;
}

idInfoStation::~idInfoStation(void)
{
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}

	repairNode.Remove();
}

void idInfoStation::Spawn(void)
{
	infoState = INFOSTAT_IDLE;

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT);

	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	idleSmoke = NULL;
	lastFTLPauseState = false;
	hasSetupSelfMarker = false;

	PostEventMS(&EV_PostSpawn, 0);

	DoGenericImpulse(BUTTON_MAP);

	//spawn the hackbox.
	//if (1)
	//{
	//	idVec3 forward, right, up;
	//	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	//	idVec3 hackboxPos = GetPhysics()->GetOrigin() + (forward * 1.5f) + (up * -13.5f) + (right * 5.5f);
	//
	//	
	//	idDict args;
	//	args.Set("classname", "env_lever3");
	//	args.Set("model", "models/objects/hackbox/hackbox.ase");
	//	args.SetInt("frobindex", HACKBOX_FROBINDEX);
	//	args.SetInt("health", 1);
	//	args.SetVector("origin", hackboxPos);
	//	args.SetMatrix("rotation", GetPhysics()->GetAxis());
	//	args.Set("displayname", "CONTROL BOX");
	//	args.SetBool("jockey_frob", false);
	//	gameLocal.SpawnEntityDef(args, &hackboxEnt);
	//
	//	if (hackboxEnt)
	//	{
	//		hackboxEnt->GetPhysics()->GetClipModel()->SetOwner(this);
	//	}
	//
	//	hackboxHacked = false;
	//	soulUpdateTimer = 0;
	//}

	soulUpdateTimer = 0;
	trackingHuman = false;
	trackingCat = false;	
	
	initialLockdoorDisplayTimer = gameLocal.time + 1000;
	initialLockdoorDisplayDone = false;

	lastSaveTime = 0;
}

void idInfoStation::Event_PostSpawn(void)
{
	//Grab all the entities we'll be tracking.

	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (gameLocal.entities[i]->entityNumber == this->entityNumber || !gameLocal.entities[i]->IsType(idFTL::Type))
			continue;

		FTLDrive_ptr = gameLocal.entities[i];
		break;
	}

	if (FTLDrive_ptr.GetEntity() == NULL)
	{
		gameLocal.Error("env_infostation was unable to find a valid FTL drive. Make sure map has an FTL drive.");
	}

	idLocationEntity *locEnt = gameLocal.LocationForEntity(this);
	if (locEnt)
	{
		Event_SetGuiParm("roomname", locEnt->GetLocation());
	}

	SetupRoomLabels();

	Event_SetGuiParm("in_endgame", gameLocal.IsInEndGame() ? "1" : "0");

	BecomeActive(TH_THINK);
}

//bc This is unfortunately a copy-paste hack of idPlayer::SetupArmstatRoomLabels
void idInfoStation::SetupRoomLabels()
{
	idStr mapname;
	mapname = gameLocal.GetMapName();
	mapname.StripPath();
	mapname.StripFileExtension();
	mapname.ToLower();

	const idDecl* mapDecl = declManager->FindType(DECL_MAPDEF, mapname, false);
	const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(mapDecl);
	if (mapDef)
	{
		for (int i = 0; i < ROOMLABELCOUNT; i++)
		{
			idStr roomVarName = idStr::Format("roomlabel%d", i);

			idStr roomLabel = mapDef->dict.GetString(roomVarName.c_str());
			if (roomLabel.Length() > 0)
			{
				Event_SetGuiParm(roomVarName.c_str(), roomLabel.c_str());

				idStr roomXVarName = idStr::Format("roomlabel%d_x", i);
				idStr roomYVarName = idStr::Format("roomlabel%d_y", i);
				Event_SetGuiFloat(roomXVarName.c_str(), mapDef->dict.GetFloat(roomXVarName));
				Event_SetGuiFloat(roomYVarName.c_str(), mapDef->dict.GetFloat(roomYVarName));
			}
			else
			{
				Event_SetGuiParm(roomVarName.c_str(), " ");
			}
		}
	}
}

void idInfoStation::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( infoState ); // int infoState

	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke

	savefile->WriteObject( FTLDrive_ptr ); // idEntityPtr<idEntity> FTLDrive_ptr
	savefile->WriteBool( lastFTLPauseState ); // bool lastFTLPauseState

	savefile->WriteBool( hasSetupSelfMarker ); // bool hasSetupSelfMarker

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteObject( hackboxEnt ); // idEntity * hackboxEnt
	savefile->WriteInt( soulUpdateTimer ); // int soulUpdateTimer

	savefile->WriteBool( trackingHuman ); // bool trackingHuman
	savefile->WriteBool( trackingCat ); // bool trackingCat

	savefile->WriteBool( initialLockdoorDisplayDone ); // bool initialLockdoorDisplayDone
	savefile->WriteInt( initialLockdoorDisplayTimer ); // int initialLockdoorDisplayTimer
}

void idInfoStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( infoState ); // int infoState

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke

	savefile->ReadObject( FTLDrive_ptr ); // idEntityPtr<idEntity> FTLDrive_ptr
	savefile->ReadBool( lastFTLPauseState ); // bool lastFTLPauseState

	savefile->ReadBool( hasSetupSelfMarker ); // bool hasSetupSelfMarker

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadObject( hackboxEnt ); // idEntity * hackboxEnt
	savefile->ReadInt( soulUpdateTimer ); // int soulUpdateTimer

	savefile->ReadBool( trackingHuman ); // bool trackingHuman
	savefile->ReadBool( trackingCat ); // bool trackingCat

	savefile->ReadBool( initialLockdoorDisplayDone ); // bool initialLockdoorDisplayDone
	savefile->ReadInt( initialLockdoorDisplayTimer ); // int initialLockdoorDisplayTimer

	lastSaveTime = 0;
}

void idInfoStation::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	if (!initialLockdoorDisplayDone && gameLocal.time > initialLockdoorDisplayTimer)
	{
		initialLockdoorDisplayDone = true;
		UpdateLockedDoorDisplay();
	}

	if (gameLocal.time >= 1000 && !hasSetupSelfMarker)
	{
		//Update player position on map. We do this after a short delay to let things settle down at game start.
		hasSetupSelfMarker = true;		
		idVec3 forwardDir;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);

		idVec2 playerNormalizedPos = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetMapguiNormalizedPosition(this->GetPhysics()->GetOrigin() + forwardDir * 8);
		Event_SetGuiFloat("playerx", playerNormalizedPos.x);
		Event_SetGuiFloat("playery", playerNormalizedPos.y);

		const char *mapName = gameLocal.world->spawnArgs.GetString("mtr_image");
		if (mapName[0] != '\0')
		{
			Event_SetGuiParm("mapimage", mapName);
		}
	}

	if (gameLocal.time >= 1000)
	{
		//player icon yaw.
		float playerYaw = gameLocal.GetLocalPlayer()->viewAngles.yaw;
		Event_SetGuiFloat("playeryaw", playerYaw);
	}

	if (infoState == INFOSTAT_IDLE)
	{
		//Update the FTL timer.
		int rawTimervalue;
		rawTimervalue = static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->GetPublicTimer();
		if (rawTimervalue <= 0)
		{
			Event_SetGuiParm("ftltimer", "");
		}
		else
		{
			int seconds = rawTimervalue / 1000.0f;
			Event_SetGuiInt("ftltimer", seconds  + 1);
		}


		bool active = static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->IsJumpActive(false);
		bool countdown = static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->IsJumpActive(true);

		if (active)
			Event_SetGuiParm("ftlstatus", "JUMPING");
		else if (countdown)
			Event_SetGuiParm("ftlstatus", "CALCULATING TELEMETRY");
		else
			Event_SetGuiParm("ftlstatus", "IDLE");


		//Event_SetGuiParm("gui_ftlhealth", idStr::Format("%d", static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->normalizedHealth));
		//
		//if (static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->GetPipePauseState())
		//{
		//	if (!lastFTLPauseState)
		//	{
		//		lastFTLPauseState = true;
		//		Event_SetGuiInt("pauseactive", 1);
		//	}
		//
		//	Event_SetGuiParm("gui_ftlpausetimer", gameLocal.ParseTimeMS(static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->GetPublicPauseTimer()));
		//}
		//else if (!static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->GetPipePauseState() && lastFTLPauseState)
		//{
		//	lastFTLPauseState = false;
		//	Event_SetGuiInt("pauseactive", 0);
		//}

		UpdateSoulsLocation();
	}
	

	idStaticEntity::Think();
}

//When player hits gui buttons.
void idInfoStation::DoGenericImpulse(int index)
{
	if (index == BUTTON_MAP)
	{
		//Map button.
		SetLight(true, idVec3(.3f, .5f, .6f));
		return;
	}
	else if (index == BUTTON_FTL)
	{
		//FTL button.
		SetLight(true, idVec3(0, .3f, .5f));
		return;
	}
    else if (index == BUTTON_REPAIRBOT)
    {
        SummonRepairbot();
        return;
    }
	else if (index == BUTTON_SAVEGAME)
	{
		if (lastSaveTime + 1000 < gameLocal.hudTime)
		{
			lastSaveTime = gameLocal.hudTime;

			//BC 4-15-2025: combat state check.
			//only allow save when world is in non-combatstate.
			if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT)
			{
				//Tell player they can't save during combat state.
				StartSound("snd_saveerror", SND_CHANNEL_BODY);
				Event_GuiNamedEvent(1, "onSaveCombat");

				Event_SetGuiParm("repairstatus", common->GetLanguageDict()->GetString("#str_gui_doorframe_infopanel_100045")); /*Use WALKIE-TALKIE or BRIDGE SECURITY MIC to end combat alert.*/
				Event_GuiNamedEvent(1, "updateRepairStatus");
			}
			else
			{
				//Do the save.

				cmdSystem->BufferCommandText(CMD_EXEC_APPEND, va("savegamesession", name.c_str()));

				//TODO: get confirmation whether save was successful before doing the confirmation gui
				Event_SetGuiParm("repairstatus", common->GetLanguageDict()->GetString("#str_gui_spectate_100510")); /*game saved*/
				Event_GuiNamedEvent(1, "updateRepairStatus");

				//BC 4-16-2025: distinct sound when doing a save.
				StartSound("snd_savesuccess", SND_CHANNEL_BODY);
			}
		}
		/*game saved*/	
		//if (idSessionLocal::SaveGameSession(args))
		//
		//	//make gui confirmation.
		//	Event_SetGuiParm("repairstatus", "#str_gui_spectate_100510"); /*game saved*/			
		//}
		//else
		//{
		//	//failed to save...
		//	Event_SetGuiParm("repairstatus", "#str_gui_spectate_100511"); /*error: save failed*/			
		//}

		return;
	}

	OnClickLegendButton(index);
}

void idInfoStation::SummonRepairbot()
{
    //Iterate over items in the room.
    //Mark things for repair.

    idVec3 forward;
    this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
    idVec3 myPos = GetPhysics()->GetOrigin() + (forward * 4);    

    int value = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->RepairEntitiesInRoom(myPos);

    if (value == REPRESULT_SUCCESS)
    {
		gameLocal.AddEventLog("#str_def_gameplay_inforepairrequest", GetPhysics()->GetOrigin());
        Event_SetGuiParm("repairstatus", "#str_def_gameplay_repair_summoned");
    }
    else if (value == REPRESULT_ZERO_REPAIRABLES)
    {
        Event_SetGuiParm("repairstatus", "#str_def_gameplay_repair_cantfind");
    }
    else if (value == REPRESULT_ALREADY_REPAIRBOT)
    {
        Event_SetGuiParm("repairstatus", "#str_def_gameplay_repair_enroute");
    }
    else
    {
        Event_SetGuiParm("repairstatus", "#str_def_gameplay_repair_fail");
    }

    Event_GuiNamedEvent(1, "updateRepairStatus");
}

void idInfoStation::OnClickLegendButton(int index)
{
	//legendtitle
	//legendroomlist

	trackingHuman = false;
	trackingCat = false;

	idStr classname;
	if (index == BUTTON_INFOSTATIONS)
	{
		classname = "env_infostation";
		Event_SetGuiParm("legendtitle", "#str_def_gameplay_infostations"); /*info stations*/
	}
	else if (index == BUTTON_HEALTHSTATIONS)
	{
		classname = "env_healthstation";
		Event_SetGuiParm("legendtitle", "#str_def_gameplay_healthstations"); /*health stations*/
	}
	else if (index == BUTTON_OXYGENSTATIONS)
	{
		classname = "env_toilet";
		Event_SetGuiParm("legendtitle", "#str_def_gameplay_toilets"); /*toilets*/
	}
	else if (index == BUTTON_ACCESSPANELS)
	{
		classname = "env_maintpanel*";
		Event_SetGuiParm("legendtitle", "#str_gui_vr_fusebox_100287"); /*fuseboxes*/
	}
	else if (index == BUTTON_TRASH)
	{
		classname = "env_trashchute";
		Event_SetGuiParm("legendtitle", "#str_def_gameplay_trash_name"); /*trash chutes*/
	}
	else if (index == BUTTON_HUMAN)
	{
		trackingHuman = true;
		trackingCat = false;
		Event_GuiNamedEvent(1, (trackingHuman ? "track_human_on" : "track_human_off"));
		Event_SetGuiParm("legendtitle", "#str_def_gameplay_humans"); /*humans*/
		classname = "";

		if (!trackingHuman)
		{
			Event_SetGuiParm("humancount", "");
		}
	}
	else if (index == BUTTON_CAT)
	{
		trackingHuman = false;
		trackingCat = true;
		Event_GuiNamedEvent(1, (trackingCat ? "track_cat_on" : "track_cat_off"));
		Event_SetGuiParm("legendtitle", "#str_gui_info_station_100074"); /*cats*/
		classname = "";

		if (!trackingCat)
		{
			Event_SetGuiParm("catcount", "");
		}
	}
	else if (index == BUTTON_LOSTANDFOUND)
	{
		classname = "env_lostandfound";
		Event_SetGuiParm("legendtitle", "#str_gui_info_station_100075"); /*lost & found*/
	}
	else
		return; //invalid.... exit here.


	int itemsMarked = 0;

	bool doWildcardsearch = false;
	if (classname.Find('*') >= 0)
	{
		classname.Strip('*'); //remove the wildcard.
		doWildcardsearch = true;
	}

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->spawnArgs.GetString("classname")[0] == '\0')
			continue;

		bool isMatch;
		if (doWildcardsearch)
		{
			//wildcard filter.			
			idStr currentClassname = ent->spawnArgs.GetString("classname");
			isMatch = (currentClassname.Find(classname) >= 0);
		}
		else
		{
			//regular search.
			isMatch = (idStr::Icmp(ent->spawnArgs.GetString("classname"), classname) == 0);
		}

		if (isMatch)
		{
			//Match.
			//idLocationEntity *locEnt = gameLocal.LocationForPoint(ent->GetPhysics()->GetOrigin());
			//if (locEnt)
			{
				if (index == BUTTON_ACCESSPANELS)
				{
					//Maintpanel.

					//If it's already done, then don't draw it.
					if (ent->IsType(idMaintPanel::Type))
					{
						if (static_cast<idMaintPanel *>(ent)->IsDone())
						{
							continue; //panel is already done. skip it.
						}
					}

					//For maintpanels, we have a text label attached to it. Prints name of the panel.
					Event_SetGuiParm(idStr::Format("loc%d_name", itemsMarked), ent->displayName.c_str()); //add displayname label.
				}
				else
				{
					Event_SetGuiParm(idStr::Format("loc%d_name", itemsMarked), ""); //make label blank.
				}

				//Set position on gui.
				idVec2 normalizedPos = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetMapguiNormalizedPositionViaEnt(ent);
				Event_SetGuiFloat(idStr::Format("loc%d_x", itemsMarked), normalizedPos.x);
				Event_SetGuiFloat(idStr::Format("loc%d_y", itemsMarked), normalizedPos.y);
				Event_SetGuiInt(idStr::Format("loc%d_vis", itemsMarked), 1); //make the marker visible.


				



				itemsMarked++;

				if (itemsMarked > MAX_LOCMARKERS)
				{
					//Uh oh. too many elements for the map gui to handle.
					gameLocal.Warning("infostation: too many '%s' in map (max is %d). Remove some entities.\n",  classname.c_str(), MAX_LOCMARKERS);
					return;
				}
			}			
		}
	}

	if (itemsMarked < MAX_LOCMARKERS)
	{
		for (int i = itemsMarked; i < MAX_LOCMARKERS; i++)
		{
			Event_SetGuiInt(idStr::Format("loc%d_vis", i), 0); //make the marker visible.
		}
	}
}



void idInfoStation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idDict args;

	Event_GuiNamedEvent(1, "onDamaged");

	if (infoState == INFOSTAT_DAMAGED)
		return;

	health = 0;
	gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

	//blow it up. TODO: explosion effect
	gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());
	//SetSkin(declManager->FindSkin("skins/objects/tutorialstation/broken"));
	StartSound("snd_explode", SND_CHANNEL_BODY2);

	StopSound(SND_CHANNEL_AMBIENT);
	//StartSound("snd_broken", SND_CHANNEL_BODY, 0, false, NULL);

	if (idleSmoke == NULL)
	{
		idVec3 forward;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

		args.Clear();
		args.Set("model", "machine_damaged_smoke.prt");
		args.Set("start_off", "0");
		idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));		
		idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 12) + (forward * 4));

		idAngles angles = GetPhysics()->GetAxis().ToAngles();
		angles.pitch += 90;
		idleSmoke->SetAxis(angles.ToMat3());
	}
	else
	{
		idleSmoke->SetActive(true);
	}

	SetLight(false, vec3_zero);	//turn off light.

	infoState = INFOSTAT_DAMAGED;
	needsRepair = true;
	repairrequestTimestamp = gameLocal.time;
}


void idInfoStation::SetToMode(int value)
{
	//Switch to FTL screen.
	if (value >= 1)
	{
		Event_GuiNamedEvent(1, "do_ftl_mode");
	}
	else
	{
		Event_GuiNamedEvent(1, "do_map_mode");
	}
}

void idInfoStation::SetLight(bool value, idVec3 color)
{
	if (value)
	{
		if (headlightHandle == -1)
		{
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 32.0f;
			headlight.shaderParms[0] = color.x; // R
			headlight.shaderParms[1] = color.y; // G
			headlight.shaderParms[2] = color.z; // B
			headlight.shaderParms[3] = 1.0f;
			headlight.noShadows = true;
			headlight.isAmbient = false;
			headlight.axis = mat3_identity;
			headlight.origin = GetPhysics()->GetOrigin() + (forward * 16);
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
		}
		else
		{
			headlight.shaderParms[0] = color.x; // R
			headlight.shaderParms[1] = color.y; // G
			headlight.shaderParms[2] = color.z; // B
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
	}
	else
	{
		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
			headlightHandle = -1;
		}
	}
}

void idInfoStation::DoRepairTick(int amount)
{
	UpdateVisuals();
	infoState = INFOSTAT_IDLE;

	SetToMode(BUTTON_MAP);

	Event_GuiNamedEvent(1, "onRepaired");

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}

	health = maxHealth;
	needsRepair = false;

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT);

	//Remove decals. This is to remove bulletholes/etc from covering the gui.
	if (modelDefHandle >= 0)
	{
		gameRenderWorld->RemoveDecals(modelDefHandle);
	}
}

bool idInfoStation::DoFrob(int index, idEntity * frobber)
{
	if (index != HACKBOX_FROBINDEX)
		return idEntity::DoFrob(index, frobber);

	if (frobber != gameLocal.GetLocalPlayer())
		return false;

	//if (hackboxEnt)
	//{
	//	hackboxHacked = true;
	//	hackboxEnt->PostEventMS(&EV_Remove, 0);
	//	
	//	//do the hack.
	//	Event_GuiNamedEvent(1, "onHackbox");
	//}
	
	return true;
}

void idInfoStation::UpdateSoulsLocation()
{
	if (gameLocal.time < soulUpdateTimer)
		return;

	soulUpdateTimer = gameLocal.time + 300;

	int itemsMarked = 0;

	//Iterate over all the bad guys.
	
	if (trackingHuman)
	{
		int humancount = 0;
		for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
		{
			if (!entity->IsActive() || entity->IsHidden() || entity->team != TEAM_ENEMY || entity->health <= 0)
				continue;

			if (!entity->IsType(idAI::Type))
				continue;

			//BC 2-22-2025: cull out swordfishes.
			if (!entity->spawnArgs.GetBool("can_talk", "1"))
				continue;

			//Mark position on map.
			idVec2 normalizedPos = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetMapguiNormalizedPosition(entity->GetPhysics()->GetOrigin());
			Event_SetGuiFloat(idStr::Format("soul%d_x", itemsMarked), normalizedPos.x);
			Event_SetGuiFloat(idStr::Format("soul%d_y", itemsMarked), normalizedPos.y);
			Event_SetGuiInt(idStr::Format("soul%d_vis", itemsMarked), 1); //make the marker visible.
			Event_SetGuiParm(idStr::Format("soul%d_name", itemsMarked), entity->displayName.c_str());
			itemsMarked++;

			humancount++;

			if (itemsMarked > MAX_LOCMARKERS)
			{
				//Uh oh. too many elements for the map gui to handle.
				gameLocal.Warning("diagnostic: too many soul markers in map (max is %d).\n", MAX_LOCMARKERS);
				return;
			}
		}

		Event_SetGuiInt("humancount", humancount);
	}

	if (trackingCat)
	{
		int catcount = 0;
		for (idEntity* cageEnt = gameLocal.catcageEntities.Next(); cageEnt != NULL; cageEnt = cageEnt->catcageNode.Next())
		{
			if (!cageEnt)
				continue;

			if (!cageEnt->IsType(idCatcage::Type))
				continue;

			if (static_cast<idCatcage *>(cageEnt)->IsOpened())
				continue;

			//Mark position on map.
			idVec2 normalizedPos = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetMapguiNormalizedPositionViaEnt(cageEnt);
			Event_SetGuiFloat(idStr::Format("soul%d_x", itemsMarked), normalizedPos.x);
			Event_SetGuiFloat(idStr::Format("soul%d_y", itemsMarked), normalizedPos.y);
			Event_SetGuiInt(idStr::Format("soul%d_vis", itemsMarked), 1); //make the marker visible.
			//Event_SetGuiParm(idStr::Format("soul%d_name", itemsMarked), cageEnt->displayName.c_str());
			Event_SetGuiParm(idStr::Format("soul%d_name", itemsMarked), cageEnt->spawnArgs.GetString("name_cat", "???"));
			itemsMarked++;

			catcount++;

			if (itemsMarked > MAX_LOCMARKERS)
			{
				//Uh oh. too many elements for the map gui to handle.
				gameLocal.Warning("diagnostic: too many soul markers in map (max is %d).\n", MAX_LOCMARKERS);
				return;
			}
		}

		Event_SetGuiInt("catcount", catcount);
	}

	if (itemsMarked < MAX_LOCMARKERS)
	{
		for (int i = itemsMarked; i < MAX_LOCMARKERS; i++)
		{
			Event_SetGuiInt(idStr::Format("soul%d_vis", i), 0); //make the marker hidden.
		}
	}

	//Event_SetGuiInt("soulscount", itemsMarked);
}

void idInfoStation::DoDelayedDoorlockDisplayUpdate()
{
	//When we update, we need a brief delay so the doors are correctly marked as unbarricaded. If we do it on the same frame as the barricade unlock, it
	//doesn't correctly update the locked door display.
	initialLockdoorDisplayTimer = gameLocal.time + 300;
	initialLockdoorDisplayDone = false;
}

void idInfoStation::UpdateLockedDoorDisplay()
{
	//Update the display of locked doors.

	int markedDoors = 0;

	//Iterate through doors.
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden() || !ent->IsType(idDoor::Type) || ent->IsType(idVentdoor::Type))
			continue;

		if (!static_cast<idDoor *>(ent)->IsBarricaded())
			continue;		

		//found a locked door.
		idVec2 normalizedPos = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetMapguiNormalizedPositionViaEnt(ent);
		Event_SetGuiFloat(idStr::Format("doorlock%d_x", markedDoors), normalizedPos.x);
		Event_SetGuiFloat(idStr::Format("doorlock%d_y", markedDoors), normalizedPos.y);
		Event_SetGuiInt(idStr::Format("doorlock%d_vis", markedDoors), 1); //make the marker visible.

		//Figure out what color to draw.
		idVec3 barricadeColor = static_cast<idDoor *>(ent)->GetBarricadeColor();
		Event_SetGuiFloat(idStr::Format("doorlock%d_r", markedDoors), barricadeColor.x);
		Event_SetGuiFloat(idStr::Format("doorlock%d_g", markedDoors), barricadeColor.y);
		Event_SetGuiFloat(idStr::Format("doorlock%d_b", markedDoors), barricadeColor.z);

		markedDoors++;

		if (markedDoors > MAX_DOORLOCKMARKERS)
		{
			//Uh oh. too many elements for the map gui to handle.
			gameLocal.Warning("diagnostic: too many door lock markers in map (max is %d).\n", MAX_DOORLOCKMARKERS);
			break;
		}
	}

	if (markedDoors < MAX_DOORLOCKMARKERS)
	{
		for (int i = markedDoors; i < MAX_DOORLOCKMARKERS; i++)
		{
			Event_SetGuiInt(idStr::Format("doorlock%d_vis", i), 0); //make the marker hidden.
		}
	}
}