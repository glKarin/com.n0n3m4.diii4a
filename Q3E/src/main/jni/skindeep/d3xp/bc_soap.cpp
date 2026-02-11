#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

//#include "item.h"

#include "bc_soap.h"

#define SETTLE_TIME 2000 //how long to ignore physics at game start
#define COLLIDESPAWN_THROWFORCE 48
#define COLLISION_THRESHOLD 50 //only collide if velocity is over this value

#define SPAWNTIME_GRACEPERIOD 1500

CLASS_DECLARATION(idMoveableItem, idSoap)
END_CLASS

idSoap::idSoap(void)
{
	//spawnTime = 0;
}

idSoap::~idSoap(void)
{
}

void idSoap::Spawn(void)
{
	//spawnTime = gameLocal.time;
}

void idSoap::Save(idSaveGame *savefile) const
{
	// savegame no data
}

void idSoap::Restore(idRestoreGame *savefile)
{
	// savegame no data
}

bool idSoap::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (IsHidden())
		return true;

	if (gameLocal.time < SETTLE_TIME) //ignore game start initialization.
	{
		return idMoveableItem::Collide(collision, velocity);
	}

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
	SpawnSud(collision.c.normal, COLLIDESPAWN_THROWFORCE);

	return true;
}

bool idSoap::DoFrob(int index, idEntity* frobber)
{
	if (index == CARRYFROB_INDEX && frobber == gameLocal.GetLocalPlayer())
	{
		//special frob when carrying the item.
		//gameLocal.DoSpewBurst(this, SPEWTYPE_DEODORANT);

		bool isSmelly = gameLocal.GetLocalPlayer()->GetSmelly();
		if (isSmelly)
		{
			gameLocal.GetLocalPlayer()->SetSmelly(false, true);
		}

		gameLocal.GetLocalPlayer()->DropCarryable(this);

		Hide();
		PostEventMS(&EV_Remove, 0);
		SpawnSud(gameLocal.GetLocalPlayer()->viewAngles.ToForward(), 200);

		return true;
	}

	return idMoveableItem::DoFrob(index, frobber);
}

void idSoap::SpawnSud(idVec3 throwDir, float throwForce)
{
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
			throwDir *= throwForce;
			ent->GetPhysics()->SetLinearVelocity(throwDir);
		}
	}
}

