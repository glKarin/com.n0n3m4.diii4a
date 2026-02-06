//#include "sys/platform.h"
//#include "Entity.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "Item.h"
#include "bc_frobcube.h"
#include "bc_vendingmachine.h"

const int VENDING_DISPENSETIME = 1750;
const idVec3 COLOR_IDLE = idVec3(0, 1, 0);
const idVec3 COLOR_BUSY = idVec3(1, .7f, 0);

const idVec3 BUTTONOFFSET = idVec3(17, 3, 58.5f);

#define DEATHSPEW_INTERVAL 400

CLASS_DECLARATION(idStaticEntity, idVendingmachine)
END_CLASS

idVendingmachine::idVendingmachine(void)
{
	itemDeathSpewCounter = 0;
	itemDeathSpewTimer = 0;	

	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);

	vendState = 0;
	stateTimer = 0;

	frobbutton1 = nullptr;
    itemDef = nullptr;
	totalItemCounter = 0;
}

idVendingmachine::~idVendingmachine(void)
{
	repairNode.Remove();
}

void idVendingmachine::Spawn(void)
{
	idDict args;
		

	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);	 //idle hum.
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);	
	
	
	args.Clear();
	args.Set("model", "models/objects/vendingmachine/vendingmachine_button_cm.ase");
	args.SetVector("cursoroffset", BUTTONOFFSET);
	args.SetInt("health", 1);
	frobbutton1 = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	frobbutton1->SetOrigin(GetPhysics()->GetOrigin());
	frobbutton1->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	frobbutton1->GetPhysics()->GetClipModel()->SetOwner(this);
	frobbutton1->Bind(this, false);
	//static_cast<idFrobcube*>(frobbutton1)->SetIndex(1);

	vendState = VENDSTATE_IDLE;
	SetColor(COLOR_IDLE);


	needsRepair = false;
	repairrequestTimestamp = 0;


	renderEntity.shaderParms[7] = 1; //make sign turn on.
	UpdateVisuals();


	itemDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_itemspawn"), false);
	if (!itemDef)
	{
		common->Error("Vending machine '%s' unable to find def_itemspawn '%s'\n", this->GetName(), spawnArgs.GetString("def_itemspawn"));
		return;
	}


	if (!spawnArgs.GetBool("start_on", "1"))
	{
		//make it start off (dead).
		SetColor(idVec3(0, 0, 0));
		renderEntity.shaderParms[7] = 0; //make sign turn off.
		UpdateVisuals();
		vendState = VENDSTATE_DISABLED;
		StopSound(SND_CHANNEL_BODY, false);
	}

	totalItemCounter = spawnArgs.GetInt("totalitems");
}



void idVendingmachine::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( vendState ); // int vendState
	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteObject( frobbutton1 ); // idEntity* frobbutton1

	savefile->WriteEntityDef( itemDef ); // const idDeclEntityDef	* itemDef

	savefile->WriteInt( itemDeathSpewTimer ); // int itemDeathSpewTimer
	savefile->WriteInt( itemDeathSpewCounter ); // int itemDeathSpewCounter

	savefile->WriteInt( totalItemCounter ); // int totalItemCounter
}

void idVendingmachine::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( vendState ); // int vendState
	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadObject( frobbutton1 ); // idEntity* frobbutton1

	savefile->ReadEntityDef( itemDef ); // const idDeclEntityDef	* itemDef

	savefile->ReadInt( itemDeathSpewTimer ); // int itemDeathSpewTimer
	savefile->ReadInt( itemDeathSpewCounter ); // int itemDeathSpewCounter

	savefile->ReadInt( totalItemCounter ); // int totalItemCounter
}

void idVendingmachine::Think(void)
{	
	if (vendState == VENDSTATE_DELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			if (totalItemCounter > 0)
			{
				bool successfulSpawn = SpawnTheItem();
				if (!successfulSpawn)
				{
					StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
				}
			}
			else
			{
				//No more items to dispense.
				StartSound("snd_empty", SND_CHANNEL_ANY, 0, false, NULL);
			}
			
			SetColor(COLOR_IDLE);
			frobbutton1->isFrobbable = true;
			vendState = VENDSTATE_IDLE;
			BecomeInactive(TH_THINK);

			if (totalItemCounter > 0)
				totalItemCounter--;
		}
	}

	if (itemDeathSpewCounter > 0 && gameLocal.time > itemDeathSpewTimer)
	{
		if (totalItemCounter <= 0)
		{
		}
		else
		{
			itemDeathSpewTimer = gameLocal.time + DEATHSPEW_INTERVAL;
			SpawnTheItem();
			itemDeathSpewCounter--;
			totalItemCounter--;
		}
	}

	idStaticEntity::Think();
}

bool idVendingmachine::SpawnTheItem()
{
	//Spawn the item.
	idEntity *spawnedItem;
	idVec3 forward, right, up;
	idVec3 candidateSpawnPos;

	gameLocal.SpawnEntityDef(itemDef->dict, &spawnedItem, false);
	if (spawnedItem)
	{
		//Find a suitable place to spawn the item.	
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		candidateSpawnPos = this->GetPhysics()->GetOrigin() + (forward * 22) + (up * 43) + (right * 14);

		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, candidateSpawnPos, candidateSpawnPos, spawnedItem->GetPhysics()->GetBounds(), MASK_SOLID, NULL);

		if (boundTr.fraction >= 1.0f)
		{
			//all clear.
			spawnedItem->SetOrigin(candidateSpawnPos);
			gameLocal.DoParticle("smoke_ring13.prt", candidateSpawnPos);

			if (spawnedItem->IsType(idItem::Type))
			{
				static_cast<idItem *>(spawnedItem)->SetJustDropped(true);
			}

			//eject it out a little. Mainly so that it looks better when ejecting during zero g.
			float randomRightAmount = gameLocal.random.RandomInt(-24, 24);
			spawnedItem->GetPhysics()->SetLinearVelocity((forward * 64) + (up * 24) + (right * randomRightAmount));
			spawnedItem->GetPhysics()->SetAngularVelocity(idVec3(32, 0, 0));

			return true;
		}
		else
		{
			spawnedItem->PostEventMS(&EV_Remove, 0);
		}
	}

	return false;
}



//void idVendingmachine::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//}

bool idVendingmachine::DoFrob(int index, idEntity * frobber)
{
	//idVec3 forward, right, up;

	if (vendState == VENDSTATE_DISABLED)
	{
		StartSound("snd_button", SND_CHANNEL_VOICE, 0, false, NULL);
		return false;
	}

	if (vendState != VENDSTATE_IDLE)
		return false;
	
	StartSound("snd_button", SND_CHANNEL_VOICE, 0, false, NULL);

	if (health <= 0)
		return false; //machine is busted, don't continue.

	if (totalItemCounter > 0)
		StartSound("snd_dispense", SND_CHANNEL_VOICE2, 0, false, NULL);

	vendState = VENDSTATE_DELAY;
	stateTimer = gameLocal.time + VENDING_DISPENSETIME;
	BecomeActive(TH_THINK);
	frobbutton1->isFrobbable = false;

	//this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	//gameLocal.DoParticle("smoke_ring03.prt", this->GetPhysics()->GetOrigin() + (forward * BUTTONOFFSET.x) + (right * BUTTONOFFSET.y) + (up * BUTTONOFFSET.z));

	SetColor(COLOR_BUSY);

	return false;
}

void idVendingmachine::DoRepairTick(int amount)
{
	SetColor(COLOR_IDLE);

	//Repair is done!
	health = maxHealth;
	needsRepair = false;
	vendState = VENDSTATE_IDLE;
	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);	 //idle hum.
	fl.takedamage = true;

	renderEntity.shaderParms[7] = 1; //make sign turn on.
	UpdateVisuals();
}

void idVendingmachine::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idVec3 forward;
	idAngles particleAng;

	if (!fl.takedamage)
		return;

	fl.takedamage = false;
	SetColor(idVec3(0, 0, 0));
	StopSound(SND_CHANNEL_BODY, false);
	StartSound("snd_powerdown", SND_CHANNEL_BODY, 0, false, NULL);

	if (vendState != VENDSTATE_DISABLED)
	{
		needsRepair = true;
		repairrequestTimestamp = gameLocal.time;
	}
	
	renderEntity.shaderParms[7] = 0; //make sign turn off.
	UpdateVisuals();

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idEntityFx::StartFx("fx/explosion_tnt", this->GetPhysics()->GetOrigin() + (forward * 20) + idVec3(0, 0, 48), mat3_identity, this, false);

	particleAng = this->GetPhysics()->GetAxis().ToAngles();
	particleAng.pitch -= 90;
	gameLocal.DoParticle("vent_purgesmoke.prt", this->GetPhysics()->GetOrigin() + (forward * 18) + idVec3(0, 0, 64), particleAng.ToForward());

	itemDeathSpewCounter = spawnArgs.GetInt("deathspew_count", "5");
	itemDeathSpewTimer = gameLocal.time + DEATHSPEW_INTERVAL;
	BecomeActive(TH_THINK);
}