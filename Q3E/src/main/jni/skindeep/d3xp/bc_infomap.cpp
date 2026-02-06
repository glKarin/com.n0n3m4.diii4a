#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
//#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
#include "bc_ftl.h"

#include "bc_meta.h"
#include "bc_infomap.h"

const int REBOOT_TIME = 5000;

CLASS_DECLARATION(idStaticEntity, idInfoMap)
	//EVENT(EV_Activate, idInfoStation::Event_Activate)
	EVENT(EV_PostSpawn, idInfoMap::Event_PostSpawn)
END_CLASS

idInfoMap::idInfoMap(void)
{
}

idInfoMap::~idInfoMap(void)
{
}

void idInfoMap::Spawn(void)
{
	infoState = INFOMAP_IDLE;
	stateTimer = 0;

	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);	

	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);	

	idleSmoke = NULL;

	PostEventMS(&EV_PostSpawn, 0);


	//BC just hide entity for now...
	GetPhysics()->SetContents(0);
	Hide();
}

void idInfoMap::Event_PostSpawn(void)
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
		gameLocal.Error("env_infomap was unable to find a valid FTL drive. Make sure map has an FTL drive.");
	}

	BecomeActive(TH_THINK);


}

void idInfoMap::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( infoState ); // bool infoState
	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke

	savefile->WriteObject( FTLDrive_ptr ); // idEntityPtr<idEntity> FTLDrive_ptr
}

void idInfoMap::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( infoState ); // bool infoState
	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke

	savefile->ReadObject( FTLDrive_ptr ); // idEntityPtr<idEntity> FTLDrive_ptr
}

void idInfoMap::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	if (infoState == INFOMAP_IDLE)
	{
		const char *timeStr;
		int rawTimervalue;

		//Update the FTL timer.
		rawTimervalue = static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->GetPublicTimer();
		timeStr = gameLocal.ParseTimeMS(rawTimervalue);
		
		Event_SetGuiParm("gui_ftltimer", timeStr);

		Event_SetGuiParm("gui_ftlhealth", idStr::Format("%d", static_cast<idFTL *>(FTLDrive_ptr.GetEntity())->normalizedHealth));
	}
	else if (infoState == INFOMAP_REBOOTING)
	{
		const char *repairStr;

		repairStr = gameLocal.ParseTimeMS((stateTimer + 700) - gameLocal.time);
		Event_SetGuiParm("gui_repairtime", repairStr);

		//is damaged. do reboot timer...
		if (gameLocal.time >= stateTimer)
		{
			StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);
			infoState = INFOMAP_IDLE;
			idleSmoke->SetActive(false);
			Event_GuiNamedEvent(1, "onRepaired");
		}
	}

	idStaticEntity::Think();
}



void idInfoMap::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idDict args;

	stateTimer = gameLocal.time + REBOOT_TIME;
	Event_GuiNamedEvent(1, "onDamaged");

	if (infoState == INFOMAP_REBOOTING)
		return;

	//blow it up. TODO: explosion effect
	gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());
	//SetSkin(declManager->FindSkin("skins/objects/tutorialstation/broken"));

	StartSound("snd_broken", SND_CHANNEL_BODY, 0, false, NULL);

	if (idleSmoke == NULL)
	{
		idVec3 forward;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

		args.Clear();
		args.Set("model", "machine_damaged_smoke.prt");
		args.Set("start_off", "0");
		idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));		
		idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 12) + (forward * 8));
	}
	else
	{
		idleSmoke->SetActive(true);
	}	

	infoState = INFOMAP_REBOOTING;
}

void idInfoMap::SetCurrentNode(int navnode, int nextNavnode)
{
	if (navnode == NAVNODE_0)
	{
		Event_GuiNamedEvent(1, "setnode0");

		if (nextNavnode == NAVNODE_1a)
			Event_GuiNamedEvent(1, "arrow0to1a");
		else
			Event_GuiNamedEvent(1, "arrow0to1b");
	}
	else if (navnode == NAVNODE_1a)
	{
		Event_GuiNamedEvent(1, "setnode1a");

		if (nextNavnode == NAVNODE_2a)
			Event_GuiNamedEvent(1, "arrow1ato2a");
		else
			Event_GuiNamedEvent(1, "arrow1ato2b");
	}
	else if (navnode == NAVNODE_1b)
	{
		Event_GuiNamedEvent(1, "setnode1b");

		if (nextNavnode == NAVNODE_2a)
			Event_GuiNamedEvent(1, "arrow1bto2a");
		else
			Event_GuiNamedEvent(1, "arrow1bto2b");
	}
	else if (navnode == NAVNODE_2a)
	{
		Event_GuiNamedEvent(1, "setnode2a");

		if (nextNavnode == NAVNODE_3a)
			Event_GuiNamedEvent(1, "arrow2ato3a");
		else
			Event_GuiNamedEvent(1, "arrow2ato3b");
	}
	else if (navnode == NAVNODE_2b)
	{
		Event_GuiNamedEvent(1, "setnode2b");

		if (nextNavnode == NAVNODE_3a)
			Event_GuiNamedEvent(1, "arrow2bto3a");
		else
			Event_GuiNamedEvent(1, "arrow2bto3b");
	}
	else if (navnode == NAVNODE_3a)
	{
		Event_GuiNamedEvent(1, "setnode3a");
		
		Event_GuiNamedEvent(1, "arrow3ato4");
	}
	else if (navnode == NAVNODE_3b)
	{
		Event_GuiNamedEvent(1, "setnode3b");

		Event_GuiNamedEvent(1, "arrow3bto4");
	}
	else if (navnode == NAVNODE_PIRATEBASE)
	{
		Event_GuiNamedEvent(1, "setnode4");

		Event_GuiNamedEvent(1, "arrow4");
	}
	
}