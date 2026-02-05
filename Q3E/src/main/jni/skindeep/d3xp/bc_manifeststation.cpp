#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "bc_ftl.h"

#include "bc_upgradecargo.h"
#include "bc_manifeststation.h"


// 2/16/1024 - Obsolete, this entity isn't used any more.


//const int THINK_INTERVAL = 500;
//const int TOTAL_ENTRIES_IN_GUI = 8; //what is the max amount of entries in security_station.gui

CLASS_DECLARATION(idStaticEntity, idManifestStation)
END_CLASS

idManifestStation::idManifestStation(void)
{
	//make it repairable.
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idManifestStation::~idManifestStation(void)
{
	repairNode.Remove();
}

void idManifestStation::Spawn(void)
{
	state = STAT_ACTIVE;

	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	idleSmoke = NULL;
	thinkTimer = 0;	
    unlocked = false;

    this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	BecomeActive(TH_THINK);

	PostEventMS(&EV_PostSpawn, 0);
}

void idManifestStation::Event_PostSpawn(void)
{
	state = STAT_AWAITINGUPDATE;
	thinkTimer = gameLocal.time + 300;
}



void idManifestStation::UpdateInfos()
{
	for (int i = 0; i < gameLocal.upgradecargoEntities.Num(); i++)
	{
		const char *cargoname = "???";
		idLocationEntity *locationEntity = NULL;

		if (static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->IsAvailable())
		{
			idVec3 forwardDir;
			gameLocal.upgradecargoEntities[i]->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);

			locationEntity = gameLocal.LocationForPoint(gameLocal.upgradecargoEntities[i]->GetPhysics()->GetOrigin() + (forwardDir * 4));

			idDict upgradeDict = static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->upgradeDef->dict;
			cargoname = upgradeDict.GetString("displayname", "");

			Event_GuiNamedEvent(1, idStr::Format("entry%d_activate", i));
		}
		else if (static_cast<idUpgradecargo *>(gameLocal.upgradecargoEntities[i])->IsDormant())
		{
			Event_GuiNamedEvent(1, idStr::Format("entry%d_deactivate", i));
		}

		Event_SetGuiParm(idStr::Format("entry%d_name", i), cargoname);
		Event_SetGuiParm(idStr::Format("entry%d_location", i), (locationEntity != NULL) ? locationEntity->GetLocation() : "???");
	}


}


void idManifestStation::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke
	savefile->WriteInt( thinkTimer ); // int thinkTimer
	savefile->WriteBool( unlocked ); // bool unlocked
}

void idManifestStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke
	savefile->ReadInt( thinkTimer ); // int thinkTimer
	savefile->ReadBool( unlocked ); // bool unlocked
}

void idManifestStation::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	if (state == STAT_ACTIVE)
	{
	}
	else if (state == STAT_AWAITINGUPDATE)
	{
		if (gameLocal.time >= thinkTimer)
		{
			state = STAT_ACTIVE;
			UpdateInfos();
		}
	}

	idStaticEntity::Think();
}



void idManifestStation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (state == STAT_DEAD)
		return;

    idDict args;

    Event_GuiNamedEvent(1, "onDamaged");

	state = STAT_DEAD;
    this->isFrobbable = false;

    needsRepair = true;
    repairrequestTimestamp = gameLocal.time;

	gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());
	StartSound("snd_broken", SND_CHANNEL_BODY, 0, false, NULL);

	if (idleSmoke == NULL)
	{
		idVec3 forward;
		idAngles angles;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		angles = this->GetPhysics()->GetAxis().ToAngles();
		angles.pitch += 90;

		args.Clear();
		args.Set("model", "machine_damaged_smoke.prt");
		args.Set("start_off", "0");
		idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));		
		idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 4) + (forward * 8));
		idleSmoke->SetAxis(angles.ToMat3());
	}
	else
	{
		idleSmoke->SetActive(true);
	}

	SetSkin(declManager->FindSkin(spawnArgs.GetString("brokenskin")));

    BecomeInactive(TH_THINK);
}

void idManifestStation::Unlock()
{
    this->isFrobbable = false;
    StartSound("snd_unlock", SND_CHANNEL_ANY, 0, false, NULL);

    Event_GuiNamedEvent(1, "event_unlock");
    unlocked = true;
}

bool idManifestStation::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	//security card check.
    if (gameLocal.RequirementMet(frobber, spawnArgs.GetString("requires"), 0))
    {
        //iterate over all security stations. unlock all of them.
        for (int e = 0; e < MAX_GENTITIES; e++)
        {
            if (!gameLocal.entities[e] || !gameLocal.entities[e]->IsType(idManifestStation::Type))
                continue;

            static_cast<idManifestStation *>(gameLocal.entities[e])->Unlock();
        }
    }
    else
    {
        //play error sound.
        StartSound("snd_locked", SND_CHANNEL_ANY, 0, false, NULL);
		Event_GuiNamedEvent(1, "missing_card");
    }

	return true;
}

void idManifestStation::DoRepairTick(int amount)
{    
    UpdateVisuals();
	
	Event_GuiNamedEvent(1, "event_unlock");		

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin")));

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}

    //Repair is done!
    health = maxHealth;
    needsRepair = false;
    BecomeActive(TH_THINK);
}