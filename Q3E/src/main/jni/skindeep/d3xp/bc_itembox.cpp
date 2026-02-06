#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "bc_meta.h"
#include "bc_itembox.h"

const int ITEMSPAWNDELAY = 600; //after frob, how long to wait until spawning item
const int OPENANIM_TOTALTIME = 800;

CLASS_DECLARATION(idAnimated, idItembox)
END_CLASS

idItembox::idItembox(void)
{
}

idItembox::~idItembox(void)
{
}

void idItembox::Spawn(void)
{
	state = CLOSED;
	isFrobbable = true;
	fl.takedamage = false;

	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	//BecomeInactive(TH_THINK);

	itemSpawnType = spawnArgs.GetString("item_spawn_type", "*");
}

void idItembox::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( state ); // int state

	savefile->WriteString( itemSpawnType ); // idString itemSpawnType
}

void idItembox::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( state ); // int state

	savefile->ReadString( itemSpawnType ); // idString itemSpawnType
}


void idItembox::Think(void)
{
	if (state == OPENING)
	{
		if (gameLocal.time >= stateTimer)
		{
			SpawnItem();			

			state = FINISHINGANIMATION;
			stateTimer = gameLocal.time + OPENANIM_TOTALTIME - ITEMSPAWNDELAY;

			if (spawnArgs.GetBool("colorchanges"))
				SetColor(idVec4(1, 0, 0, 1)); //turn red.
		}
	}
	else if (state == FINISHINGANIMATION)
	{
		if (gameLocal.time >= stateTimer)
		{
			//Move the frob cursor. So that it's positioned on the open door.
			spawnArgs.SetVector("cursoroffset", idVec3(6, 0, 4.9f));

			spawnArgs.Set("weapon", "weapon_itemboxlid");
			spawnArgs.SetBool("carryable", true);

			isFrobbable = true;
			state = OPENDONE;
			fl.takedamage = true;
			BecomeInactive(TH_THINK);
		}
	}	

	idAnimated::Think();
}

void idItembox::SpawnItem()
{
	const char *entityDefName;

	//First, see if there's a custom item we want it to spawn.
	idStr customEnt = spawnArgs.GetString("def_customitem");
	if (customEnt.Length() > 0)
	{
		entityDefName = customEnt.c_str();
	}
	else
	{
		idStrList* itemlist = NULL;
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->itemdropsTable.Get(itemSpawnType.c_str(), &itemlist);

		if (!itemlist)
		{
			gameLocal.Error("idItembox: unknown item_spawn_type %s", itemSpawnType.c_str());
			return;
		}

		if (itemlist->Num() <= 0)
			return;

		int randomIndex = gameLocal.random.RandomInt(itemlist->Num());

		entityDefName = (*itemlist)[randomIndex];
	}

	const idDeclEntityDef *itemDef = gameLocal.FindEntityDef(entityDefName, false);

	if (!itemDef)
	{
		gameLocal.Error("idItembox: failed to find def for '%s'.", entityDefName);
	}

	idEntity *itemEnt;
	gameLocal.SpawnEntityDef(itemDef->dict, &itemEnt, false);

	if (!itemEnt)
	{
		gameLocal.Error("InitializeItemSpawns(): found def but failed to spawn '%s'.", entityDefName);
	}

	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idVec3 spawnPos = GetPhysics()->GetOrigin() + forward * 9;

	itemEnt->SetOrigin(spawnPos);
	if (itemEnt->IsType(idItem::Type))
	{
		static_cast<idItem *>(itemEnt)->SetJustDropped(true); //prevent it from making an interestpoint.
	}
}

bool idItembox::DoFrob(int index, idEntity * frobber)
{
	idEntity::DoFrob(index, frobber);

	if (state == CLOSED)
	{
		//Open it up.
		state = OPENING;
		Event_PlayAnim("open", 1, false);
		isFrobbable = false;
		stateTimer = gameLocal.time + ITEMSPAWNDELAY;
		BecomeActive(TH_THINK);

		if (spawnArgs.GetBool("colorchanges"))
			SetColor(idVec4(1, .7f, 0, 1)); //turn yellow.
	}
	else if (state == OPENDONE)
	{		
		if (frobber == gameLocal.GetLocalPlayer())
		{
			//Give it to the player.

			//Check if player already has this item.
			const char *weaponName = spawnArgs.GetString("weapon");
			if (weaponName[0] != '\0')
			{
				if (gameLocal.GetLocalPlayer()->HasWeaponInInventory(weaponName, true))
				{
					//You already have this item. You can't carry more. Display a message.
					gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("alreadyhave");
				}
				else
				{
					//Spawn the item and give it to player.
					
					idEntity *lidEnt = PopOffLid();
					if (lidEnt)
					{
						//successful spawn of item.
						StartSound("snd_yank", SND_CHANNEL_ANY);
						lidEnt->DoFrob(0, gameLocal.GetLocalPlayer()); //have player frob the item.					
					}
				}
			}
		}
		
		return true;
	}

	return true;
}

idEntity *idItembox::PopOffLid()
{
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_nolid")));
	UpdateVisuals();

	fl.takedamage = false;
    isFrobbable = false;
    renderEntity.noShadow = true;
    renderEntity.shaderParms[7] = 1;
    UpdateVisuals();
	state = DEAD;

	//Spawn the cover plate.
	const idDeclEntityDef *itemDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_lid"), false);
	if (!itemDef)
		return NULL;

	idEntity *itemEnt;
	gameLocal.SpawnEntityDef(itemDef->dict, &itemEnt, false);
	if (!itemEnt)
		return NULL;

	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	itemEnt->GetPhysics()->SetOrigin(this->GetPhysics()->GetOrigin() + (forward * 6.1f) + (up * 5.0f));

	idAngles lidAngle = this->GetPhysics()->GetAxis().ToAngles();
	lidAngle.pitch -= 80;
	itemEnt->GetPhysics()->SetAxis(lidAngle.ToMat3());

	//particles.
	idVec3 particlePos = GetPhysics()->GetOrigin() + (up * 5.1f);
	idAngles particleAngle = this->GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 90;
	idEntityFx::StartFx("fx/itembox_lid", particlePos, particleAngle.ToMat3());


	return itemEnt;
}



void idItembox::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (state != OPENDONE)
		return;

	PopOffLid();	
}