//#include "sys/platform.h"
//#include "Entity.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "Item.h"
#include "Player.h"
#include "framework/Common.h"
#include "idlib/LangDict.h"
#include "bc_meta.h"
#include "bc_savestation.h"

#define SAVE_POSTDELAYTIME 900



CLASS_DECLARATION(idStaticEntity, idSaveStation)
END_CLASS



idSaveStation::idSaveStation(void)
{
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
	needsRepair = false;
	repairrequestTimestamp = 0;
	idleSmoke = NULL;
	state = SV_READY;
	stateTimer = 0;
	savebuttonDelayActive = false;
}

idSaveStation::~idSaveStation(void)
{
	repairNode.Remove();
}

void idSaveStation::Spawn(void)
{
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	isFrobbable = true;

	SetColor(colorGreen);
	UpdateVisuals();

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	BecomeActive( TH_THINK );
}

void idSaveStation::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke

	savefile->WriteBool( savebuttonDelayActive ); // bool savebuttonDelayActive
}

void idSaveStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke

	savefile->ReadBool( savebuttonDelayActive ); // bool savebuttonDelayActive
}

void idSaveStation::Think(void)
{	
	if (health > 0)
	{
		int worldState = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetCombatState();
		if (worldState != COMBATSTATE_IDLE)
		{
			if (state != SV_LOCKDOWN)
			{
				//transition to lockdown state
				Event_SetGuiInt("stationstate", (int)SV_LOCKDOWN);
				state = SV_LOCKDOWN;
				isFrobbable = true;
				SetColor(colorGreen);
			}			
		}
		else
		{
			if (state != SV_READY)
			{
				//transition to lockdown state
				Event_SetGuiInt("stationstate", (int)SV_READY);
				state = SV_READY;
				isFrobbable = true;
				SetColor(colorGreen);
			}
		}
	}


	if (state == SV_READY)
	{
		if (gameLocal.time > stateTimer && savebuttonDelayActive)
		{
			savebuttonDelayActive = false;
			SetColor(colorGreen);
			isFrobbable = true;
		}		
	}

	idStaticEntity::Think();
}



void idSaveStation::DoRepairTick(int amount)
{
	//Repair is done!
	health = maxHealth;
	needsRepair = false;
	fl.takedamage = true;
	SetColor(colorGreen);
	state = SV_READY;
	isFrobbable = true;
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin")));

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}
}

void idSaveStation::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	needsRepair = true;
	repairrequestTimestamp = gameLocal.time;
	SetColor(colorRed);
	state = SV_BROKEN;
	Event_SetGuiInt("stationstate", (int)SV_BROKEN);
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_broken")));

	if (idleSmoke == NULL)
	{
		idVec3 right;
		GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &right, NULL);

		idDict args;
		args.Clear();
		args.Set("model", spawnArgs.GetString("model_damagesmoke"));
		args.Set("start_off", "0");
		args.SetVector("origin", GetPhysics()->GetOrigin() + (right * 3.5f));
		idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));

		idAngles angles = GetPhysics()->GetAxis().ToAngles();
		angles.pitch += 90;
		idleSmoke->SetAxis(angles.ToMat3());
	}
	else
	{
		idleSmoke->SetActive(true);
	}
}

bool idSaveStation::DoFrob(int index, idEntity * frobber)
{
	if (health <= 0 || state == SV_BROKEN)
	{
		StartSound("snd_cancel", SND_CHANNEL_BODY);
		return true;
	}

	if (frobber == NULL)
		return true;

	if (frobber->entityNumber != gameLocal.GetLocalPlayer()->entityNumber) //only allow player to frob.
	{
		StartSound("snd_cancel", SND_CHANNEL_BODY);
		return true;
	}

	if (state == SV_LOCKDOWN)
	{
		Event_GuiNamedEvent(1, "onLockdown");
		return true;
	}
	

	//Player frobbed it.
	isFrobbable = false;
	stateTimer = gameLocal.time + SAVE_POSTDELAYTIME;	
	savebuttonDelayActive = true;
	StartSound("snd_save", SND_CHANNEL_BODY2);		
	Event_GuiNamedEvent(1, "onSave");
	SetColor(colorYellow);
	UpdateVisuals();

	DoSave();

	return true;
}



void idSaveStation::DoSave()
{
	//TODO: Save code.
}