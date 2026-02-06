#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
//#include "bc_ftl.h"


#include "bc_wallitem.h"


const int TARGET_DELAYTIME = 300;

CLASS_DECLARATION(idStaticEntity, idWallItem)	
	//EVENT(EV_PostSpawn, idWallItem::Event_PostSpawn)
END_CLASS

idWallItem::idWallItem(void)
{
}

idWallItem::~idWallItem(void)
{
}

void idWallItem::Spawn(void)
{	
	fl.takedamage = true;
	isFrobbable = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	
	state = IDLE;

	const idDeclEntityDef *itemDef;
	itemDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_drop"), false);

	if (!itemDef)
	{
		gameLocal.Error("wallitem '%s' unable to find def_drop definition: '%s'", this->GetName(), spawnArgs.GetString("def_drop"));
		return;
	}

	displayName = itemDef->dict.GetString("displayname", "");

	targetTimer = 0;
}


void idWallItem::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( targetTimer ); // int targetTimer
}

void idWallItem::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( targetTimer ); // int targetTimer
}

void idWallItem::Think(void)
{
	if (gameLocal.time > targetTimer && state == DEAD)
	{
		state = REMOVED;		
		DamageTargets();
		PostEventMS(&EV_Remove, 0);
	}

	idStaticEntity::Think();
}



//void idWallItem::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{	
//}

void idWallItem::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (state == DEAD)
		return;

	state = DEAD;

	idVec3 forward;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

	idEntity *newEnt = SpawnTheItem();
	if (newEnt)
	{
		//make it fly forward a bit.
		newEnt->GetPhysics()->SetLinearVelocity(forward * 32 + idVec3(0,0,4));		
		newEnt->GetPhysics()->SetAngularVelocity(idVec3(0, -32 + gameLocal.random.RandomInt(64), 0));
	}
}

void idWallItem::DamageTargets()
{
	const idKeyValue *kv;
	idVec3 forward, up, right;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	kv = this->spawnArgs.MatchPrefix("target", NULL);
	while (kv)
	{
		const char *targetName = kv->GetValue();		
		idEntity *targetEnt = gameLocal.FindEntity(targetName);
		if (targetEnt)
		{
			targetEnt->Damage(this, this, vec3_zero, "damage_suicide", 1.0f, 0);
		}

		kv = this->spawnArgs.MatchPrefix("target", kv); //Iterate to next entry.
	}
}

bool idWallItem::DoFrob(int index, idEntity * frobber)
{
	if (state == IDLE)
	{
		if (frobber != NULL)
		{
			if (frobber == gameLocal.GetLocalPlayer())
			{
				//player frobbed me.

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
						idEntity *newEnt = SpawnTheItem();
						if (newEnt)
						{
							//successful spawn of item.
							newEnt->DoFrob(0, gameLocal.GetLocalPlayer()); //have player frob the item.
						}
					}
				}
			}
			else
			{
				//something (not player) frobbed me.
				Damage(frobber, frobber, vec3_zero, "damage_generic", 1.0f, 0, 0);
			}
		}
	}

	return true;
}

//spawn the item, and remove this ent from world.
idEntity *idWallItem::SpawnTheItem()
{
	idEntity *itemEnt;
	const idDeclEntityDef *itemDef;

	state = DEAD;
	targetTimer = gameLocal.time + TARGET_DELAYTIME;
	BecomeActive(TH_THINK);

	itemDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_drop"), false);
	gameLocal.SpawnEntityDef(itemDef->dict, &itemEnt, false);
	if (itemEnt)
	{
		itemEnt->SetOrigin(this->GetPhysics()->GetOrigin());
		itemEnt->SetAxis(this->GetPhysics()->GetAxis());

		//decal effects, FX effects.
		DoDecalEffects();

		//make wallitem disappear.
		this->Hide();
		this->GetPhysics()->SetContents(0);		

		return itemEnt;
	}

	return NULL;
}

void idWallItem::DoDecalEffects()
{
	//Handle the decal effects created when the item is yanked off the wall.

	const idKeyValue *kv;
	idVec3 forward, up, right;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	kv = this->spawnArgs.MatchPrefix("def_deathfx", NULL);
	while (kv)
	{
		idStr prefix;
		const char *fxName;

		fxName = kv->GetValue();		
		prefix = kv->GetKey().Right(8); //truncate string to the last 8 characters (remove 'def_' from the string)

		//Get position.
		idVec3 relativePosition = this->spawnArgs.GetVector(idStr::Format("%s_position", prefix.c_str())); //forward, right, up.
		relativePosition = this->GetPhysics()->GetOrigin() + (forward * relativePosition.x) + (right * relativePosition.y) + (up * relativePosition.z);

		//Get angle.
		idVec3 finalAngle;
		idVec3 angleDirection = this->spawnArgs.GetVector(idStr::Format("%s_angle", prefix.c_str())); //forward, right, up.
		finalAngle = (forward * angleDirection.x) + (right * angleDirection.y) + (up * angleDirection.z);
		idAngles angle2 = finalAngle.ToAngles();
		angle2.pitch += 90; //compensate for particle system weirdness... pitch always has to be rotated 90 degrees.
		
		//Spawn the fx.
		idEntityFx::StartFx(fxName, relativePosition, angle2.ToMat3());
		

		kv = this->spawnArgs.MatchPrefix("def_deathfx", kv); //Iterate to next entry.
	}
}