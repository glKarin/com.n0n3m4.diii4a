#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"
#include "idlib/LangDict.h"

//#include "item.h"

#include "bc_food.h"


#define COLLISION_THRESHOLD 50 //only collide if velocity is over this value
#define SPAWNTIME_GRACEPERIOD 1500 //ignore collision immediately after spawn, so that it doesn't immediately splat on spawn.

#define COLLIDESPAWN_THROWFORCE 48

CLASS_DECLARATION(idMoveableItem, idFood)
END_CLASS


void idFood::Spawn(void)
{
	collideSpawn = spawnArgs.GetBool("collidespawn");
}

void idFood::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( collideSpawn );// bool collideSpawn;
}

void idFood::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( collideSpawn );// bool collideSpawn;
}

bool idFood::DoFrob(int index, idEntity * frobber)
{
	if (index == CARRYFROB_INDEX && frobber == gameLocal.GetLocalPlayer())
	{
		//special frob when carrying the item.
		
		//Give health.
		//gameLocal.GetLocalPlayer()->GiveHealthAdjustedMax(); //refill health.
		gameLocal.GetLocalPlayer()->WoundRegenerateHealthblock(); //refill health.
		

		//Do sound and particles.
		idStr eatSound = spawnArgs.GetString("snd_eat"); // Try to get bespoke eat sound
		if (eatSound[0] != '\0')
		{
			gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound(eatSound), SND_CHANNEL_ANY, 0, false, NULL);
		}
		else
		{
			gameLocal.GetLocalPlayer()->StartSound("snd_eat", SND_CHANNEL_ANY); // Fall back on player sound
		}

		idAngles eatParticleAngle = idAngles(180, 0, 0);
		gameLocal.DoParticle(spawnArgs.GetString("model_eat"), GetPhysics()->GetOrigin(), eatParticleAngle.ToForward());

		idStr foodName = displayName;
		gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_ate"), foodName.c_str()), GetPhysics()->GetOrigin());

		//remove from inventory.
		gameLocal.GetLocalPlayer()->RemoveCarryableFromInventory(this);
		PostEventMS(&EV_Remove, 0);



		//See if we need to spawn an item after the item is eaten.
		//This is for doing things like making the tuna create an empty tuna can after it's eaten.
		idStr eatSpawnDef = spawnArgs.GetString("def_eatspawn");
		if (eatSpawnDef.Length() > 0)
		{
			//check if player has at least one empty inventory slot.
			int emptyInventorySlots =  gameLocal.GetLocalPlayer()->GetEmptyInventorySlots();
			if (emptyInventorySlots > 0)
			{
				const idDeclEntityDef *itemDef;
				itemDef = gameLocal.FindEntityDef(eatSpawnDef, false);
				if (itemDef)
				{
					//found definition for the item to spawn.

					//Spawn item.
					idEntity *itemEnt;
					idDict args;
					args.Set("classname", eatSpawnDef);
					gameLocal.SpawnEntityDef(args, &itemEnt);

					if (itemEnt)
					{
						itemEnt->DoFrob(0, gameLocal.GetLocalPlayer());
					}
				}
			}
		}

		// SW: Adding functionality to generate an interestpoint if specified
		//This is for things like the potato chip bag making a loud crunch noise.
		idStr eatInterestDef = spawnArgs.GetString("interest_eat");
		if (eatInterestDef.Length() > 0)
		{
			gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), eatInterestDef.c_str());
		}

		return true;
	}	

	return idMoveableItem::DoFrob(index, frobber);	
}

//Code crime........ this is the same function as bc_soap.cpp......... I really didn't want food to be soap or vice versa.......
bool idFood::Collide(const trace_t& collision, const idVec3& velocity)
{
	if (!collideSpawn)
	{
		return idMoveableItem::Collide(collision, velocity);
	}

	if (IsHidden())
		return true;

	float v = -(velocity * collision.c.normal);
	//if it's moving slowly OR it just spawned, then don't do suds
	if (v < COLLISION_THRESHOLD || gameLocal.time - spawnTime < SPAWNTIME_GRACEPERIOD)
	{
		return idMoveableItem::Collide(collision, velocity);
	}

	//Destroy self.
	Hide();
	PostEventMS(&EV_Remove, 0);

	//Spawn suds.
	idStr defToSpawn = spawnArgs.GetString("def_collidespawn");
	if (defToSpawn.Length() > 0)
	{
		idDict args;
		args.SetVector("origin", GetPhysics()->GetOrigin());
		args.Set("classname", defToSpawn);
		idEntity* ent = NULL;
		gameLocal.SpawnEntityDef(args, &ent);
		if (ent)
		{
			//throw it a little. If this throw isn't done, we sometimes get weird collision penetration issues.
			idVec3 throwDir = collision.c.normal;
			throwDir *= COLLIDESPAWN_THROWFORCE;
			ent->GetPhysics()->SetLinearVelocity(throwDir);
		}
	}

	return true;
}
