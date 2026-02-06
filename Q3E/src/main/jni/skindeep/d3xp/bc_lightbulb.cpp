#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_meta.h"
#include "bc_lightbulb.h"



#define BREAK_THRESHOLD 400 //if collide & above this velocity, then shatter it.

#define COLLIDESPAWN_THROWFORCE 48 //for glass shards, how far to throw them

CLASS_DECLARATION(idMoveableItem, idLightbulb)
END_CLASS

void idLightbulb::Save(idSaveGame *savefile) const
{
	// savegame no data
}

void idLightbulb::Restore(idRestoreGame *savefile)
{
	// savegame no data
}


void idLightbulb::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	//GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	//BecomeActive(TH_THINK);
}





void idLightbulb::ShatterAndRemove(trace_t collision)
{
	if (health <= 0)
		return;

	health = 0;

	//Do this before removing objects from world.
	this->Hide();
	physicsObj.SetContents(0);
	

	idAngles fxAngle;
	fxAngle.yaw = gameLocal.random.RandomInt(350);
	idEntityFx::StartFx("fx/glass_shard_break", GetPhysics()->GetOrigin(), fxAngle.ToMat3());

	//Remove from world.
	PostEventMS(&EV_Remove, 0);


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
}

bool idLightbulb::JustBashed(trace_t tr)
{
	if (tr.fraction < 1)
	{
		ShatterAndRemove(tr);
	}

	return true;
}

//void idLightbulb::Think(void)
//{
//
//}





//This gets called when the glasspiece touches the world.
bool idLightbulb::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			return false;
		}
	}

	if (gameLocal.time < spawnTime + 1000)
	{
		//spawntime grace period. Do normal collision.
		return idMoveableItem::Collide(collision, velocity);
	}

	float velocityLength = velocity.LengthFast();
	if (velocityLength > BREAK_THRESHOLD)
	{
		//shatter it.
		ShatterAndRemove(collision);
	}	

	return idMoveableItem::Collide(collision, velocity);
}

