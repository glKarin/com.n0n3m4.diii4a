#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "idlib/LangDict.h"

#include "WorldSpawn.h"
#include "bc_wrench.h"


//Duper
//Duplicates objects in world.

#define THROW_REPAIRVELOCITY 50
#define REPAIR_LEEWAYDISTANCE 32

CLASS_DECLARATION(idMoveableItem, idWrench)
//EVENT(EV_Touch, idWrench::Event_Touch)
END_CLASS

void idWrench::Save(idSaveGame *savefile) const
{
	savefile->WriteVec3( lastBashPos ); // idVec3 lastBashPos
	savefile->WriteInt( damageCooldownTimer ); // int damageCooldownTimer
}

void idWrench::Restore(idRestoreGame *savefile)
{
	savefile->ReadVec3( lastBashPos ); // idVec3 lastBashPos
	savefile->ReadInt( damageCooldownTimer ); // int damageCooldownTimer
}

void idWrench::Spawn(void)
{
	//Allow player to clip through it.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	lastBashPos = vec3_zero;
	damageCooldownTimer = 0;
}

void idWrench::Think()
{
	if (gameLocal.time > damageCooldownTimer && !canDealEntDamage)
	{
		canDealEntDamage = true;
		BecomeInactive(TH_THINK);
	}

	idMoveableItem::Think();
}

bool idWrench::Collide(const trace_t &collision, const idVec3 &velocity)
{
	float v;
	v = -(velocity * collision.c.normal);
	if (v > THROW_REPAIRVELOCITY )
	{
		if (DoRepairHitLogic(collision, false))
		{
			//successful repair.
			BecomeActive(TH_THINK);
			damageCooldownTimer = gameLocal.time + 500;
			canDealEntDamage = false;
		}
	}

	return idMoveableItem::Collide(collision, velocity);
}

bool idWrench::JustBashed(trace_t tr)
{
	return DoRepairHitLogic(tr, true);
}

bool idWrench::DoRepairHitLogic(trace_t tr, bool isBash)
{
	if (gameLocal.time < 1000)
		return false;

	lastBashPos = tr.endpos;

	if (tr.c.entityNum <= MAX_GENTITIES - 2 && tr.c.entityNum >= 0)
	{
		if (gameLocal.entities[tr.c.entityNum] != NULL)
		{
			if (DoRepairOnEnt(gameLocal.entities[tr.c.entityNum], isBash))
			{
				//successful repair.
				return true;
			}
		}
	}

	//failed to find object that needs repairing.
	//Try to find nearby object.
	float closestDist = 99999;
	idEntity* entToRepair = NULL;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->needsRepair)
			continue;

		float dist = (tr.endpos - ent->GetPhysics()->GetOrigin()).Length();
		if (dist < closestDist && dist < REPAIR_LEEWAYDISTANCE)
		{
			dist = closestDist;
			entToRepair = ent;
		}
	}

	if (entToRepair != NULL)
	{
		return DoRepairOnEnt(entToRepair, isBash);
	}

	return false;
}

bool idWrench::DoRepairOnEnt(idEntity *targetEnt, bool isBash)
{
	if (targetEnt == NULL)
		return false;

	if (!targetEnt->needsRepair)
		return false;

	//don't repair something that just got damaged recently.
	//This is to handle the case where throwing the wrench damages and repairs the object on the same frame.
	if (targetEnt->lastDamageTime + 300 > gameLocal.time && !isBash)
		return false;

	StartSound("snd_repair", SND_CHANNEL_ITEM);
	gameLocal.DoParticle(spawnArgs.GetString("model_repairprt"), lastBashPos);
	targetEnt->DoRepairTick(1);	
	gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_wrench_repair"), targetEnt->displayName.c_str()), targetEnt->GetPhysics()->GetOrigin());
	return true;
}