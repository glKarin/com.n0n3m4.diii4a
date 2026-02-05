#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
#include "bc_ftl.h"

#include "bc_gunner.h"
#include "bc_securitystation.h"


// 2/16/1024 - Obsolete, this entity isn't used any more.


const int THINK_INTERVAL = 1000;
const int TOTAL_ENTRIES_IN_GUI = 8; //what is the max amount of entries in security_station.gui

CLASS_DECLARATION(idStaticEntity, idSecurityStation)
END_CLASS

idSecurityStation::idSecurityStation(void)
{
	//make it repairable.
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idSecurityStation::~idSecurityStation(void)
{
}

void idSecurityStation::Spawn(void)
{
	securityState = SECURITYSTAT_LOCKED;

	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	idleSmoke = NULL;
	thinkTimer = 0;	
    unlocked = false;

    this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	BecomeActive(TH_THINK);
}



void idSecurityStation::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( securityState ); // int securityState
	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke
	savefile->WriteInt( thinkTimer ); // int thinkTimer
	savefile->WriteBool( unlocked ); // bool unlocked
}

void idSecurityStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( securityState ); // int securityState
	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke
	savefile->ReadInt( thinkTimer ); // int thinkTimer
	savefile->ReadBool( unlocked ); // bool unlocked
}

void idSecurityStation::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	if (securityState == SECURITYSTAT_ACTIVE)
	{
		if (gameLocal.time > thinkTimer)
		{
			thinkTimer = gameLocal.time + THINK_INTERVAL;

			//gather up all the bad guys.
			int enemiesFound = 0;
			for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
			{
				if (!entity || entity->team != TEAM_ENEMY)
					continue;

				if (!entity->IsType(idAI::Type) || entity->petNode.Owner() != NULL)
					continue;

				//ok, we now have a valid enemy.				
				
				//Make the gui entry visible.
				Event_SetGuiInt(idStr::Format("entry%d_visible", enemiesFound), 1);

				idLocationEntity *locationEntity = gameLocal.LocationForPoint(entity->GetPhysics()->GetOrigin());				

				Event_SetGuiParm(idStr::Format("entry%d_name", enemiesFound), entity->spawnArgs.GetString("displayname"));
				Event_SetGuiParm(idStr::Format("entry%d_location", enemiesFound), (locationEntity != NULL) ? locationEntity->GetLocation() : "???" );
				Event_SetGuiParm(idStr::Format("entry%d_health", enemiesFound), idStr::Format("%d/%d", max(entity->health, 0), entity->maxHealth) );

				enemiesFound++;

				if (enemiesFound >= TOTAL_ENTRIES_IN_GUI)
					break; //exceeds the amount we can see in gui... just exit here.
			}

			Event_SetGuiParm("totalamount", idStr::Format("HOSTILES DETECTED: %d", enemiesFound));

			if (enemiesFound < TOTAL_ENTRIES_IN_GUI)
			{
				//hide excess gui entries.
				for (int i = enemiesFound; i < TOTAL_ENTRIES_IN_GUI; i++)
				{
					Event_SetGuiInt(idStr::Format("entry%d_visible",i), 0);
				}
			}


		}
	}

	idStaticEntity::Think();
}



void idSecurityStation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (securityState == SECURITYSTAT_DEAD)
		return;

    idDict args;

    Event_GuiNamedEvent(1, "onDamaged");

	securityState = SECURITYSTAT_DEAD;
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

void idSecurityStation::Unlock()
{
    this->isFrobbable = false;
    StartSound("snd_unlock", SND_CHANNEL_ANY, 0, false, NULL);

    Event_GuiNamedEvent(1, "event_unlock");
	securityState = SECURITYSTAT_ACTIVE;
    unlocked = true;
}

bool idSecurityStation::DoFrob(int index, idEntity * frobber)
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
            if (!gameLocal.entities[e] || !gameLocal.entities[e]->IsType(idSecurityStation::Type))
                continue;

            static_cast<idSecurityStation *>(gameLocal.entities[e])->Unlock();
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

void idSecurityStation::DoRepairTick(int amount)
{
    
    UpdateVisuals();

    securityState = unlocked ? SECURITYSTAT_ACTIVE : SECURITYSTAT_LOCKED;


	if (securityState == SECURITYSTAT_LOCKED)
	{
		Event_GuiNamedEvent(1, "event_lock"); //return to locked screen.		
		this->isFrobbable = true;
	}
	else
	{
		Event_GuiNamedEvent(1, "event_unlock");
	}
		

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