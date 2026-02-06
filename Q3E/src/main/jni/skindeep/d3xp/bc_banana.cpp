#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_food.h"
#include "bc_banana.h"



//Banana
//Turns into a banana peel when it collides with something.

#define COLLISION_VELOCITY_THRESHOLD 20

CLASS_DECLARATION(idFood, idBanana)
	EVENT(EV_Touch, idBanana::Event_Touch)
END_CLASS

void idBanana::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( spawnTime ); //  int spawnTime
	savefile->WriteBool( hasBecomePeel ); //  bool hasBecomePeel
}

void idBanana::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( spawnTime ); //  int spawnTime
	savefile->ReadBool( hasBecomePeel ); //  bool hasBecomePeel
}

void idBanana::Spawn(void)
{
	hasBecomePeel = false;
	spawnTime = gameLocal.time;
}

bool idBanana::Collide(const trace_t& collision, const idVec3& velocity)
{
	//Don't do checks if being held by player
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			return false;
		}
	}

	idMoveableItem::Collide(collision, velocity);

	//turn into a banana peel.
	float v;
	v = -(velocity * collision.c.normal);	
	if (v > COLLISION_VELOCITY_THRESHOLD && !hasBecomePeel && gameLocal.time > spawnTime + 2000) //don't make it turn into a banana peel right on spawn.
	{
		//turn into banana peel.
		hasBecomePeel = true;

		idStr eatSpawnDef = spawnArgs.GetString("def_eatspawn");
		if (eatSpawnDef.Length() > 0)
		{			
			const idDeclEntityDef* itemDef;
			itemDef = gameLocal.FindEntityDef(eatSpawnDef, false);
			if (itemDef)
			{
				//found definition for the item to spawn.
				//Spawn item.
				idEntity* itemEnt;
				idDict args;
				args.Set("classname", eatSpawnDef);
				args.SetVector("origin", GetPhysics()->GetOrigin());
				gameLocal.SpawnEntityDef(args, &itemEnt);
				if (itemEnt)
				{
					gameLocal.DoParticle(spawnArgs.GetString("model_eat"), GetPhysics()->GetOrigin());

					Hide();
					physicsObj.SetContents(0);
					PostEventMS(&EV_Remove, 0);
				}
			}			
		}

		return false;
	}

	return true;
}
