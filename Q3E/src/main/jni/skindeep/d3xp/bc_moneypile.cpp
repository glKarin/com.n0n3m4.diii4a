#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"
#include "framework/DeclEntityDef.h"

#include "bc_meta.h"
#include "bc_zena.h"
#include "bc_moneypile.h"


CLASS_DECLARATION(idTrigger_Multi, idTrigger_moneypile)
//EVENT(EV_Touch, idTrigger_moneypile::Event_Touch)
END_CLASS


#define TOUCH_DISTANCE 48


idTrigger_moneypile::idTrigger_moneypile()
{
	waitingForTrigger = true;
	thinkTimer = 0;
	zenaEnt = NULL;
}

idTrigger_moneypile::~idTrigger_moneypile(void)
{
}

void idTrigger_moneypile::Spawn()
{
	idTrigger_Multi::Spawn();
	BecomeActive(TH_THINK);

	PostEventMS(&EV_PostSpawn, 100);
}

void idTrigger_moneypile::Event_PostSpawn(void)
{
	//Keep track of Zena
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
		{
			continue;
		}
	
		if (ent->IsType(idZena::Type))
		{
			zenaEnt = ent;
			return;
		}
	}
}

void idTrigger_moneypile::Save(idSaveGame* savefile) const
{
	savefile->WriteBool( waitingForTrigger ); // bool waitingForTrigger
	savefile->WriteInt( thinkTimer ); // int thinkTimer
	savefile->WriteObject( zenaEnt ); // idEntityPtr<idEntity> zenaEnt
}
void idTrigger_moneypile::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool( waitingForTrigger ); // bool waitingForTrigger
	savefile->ReadInt( thinkTimer ); // int thinkTimer
	savefile->ReadObject( zenaEnt ); // idEntityPtr<idEntity> zenaEnt
}

void idTrigger_moneypile::Think(void)
{
	if (gameLocal.time > thinkTimer)
	{
		//do distance checks.
		float distanceToPlayer = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).Length();
		if (distanceToPlayer <= TOUCH_DISTANCE)
		{
			DoParticleBurst();
			return;
		}

		if (zenaEnt.IsValid())
		{
			float distanceToZena = (zenaEnt.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).Length();
			if (distanceToZena <= TOUCH_DISTANCE)
			{
				DoParticleBurst();
				return;
			}
		}
	}

	idTrigger_Multi::Think();
}


//void idTrigger_moneypile::Event_Touch(idEntity* other, trace_t* trace)
//{
//	if (other == nullptr)
//		return;
//
//	if (other->entityNumber != gameLocal.GetLocalPlayer()->entityNumber)
//		return;
//
//	DoParticleBurst();
//}

void idTrigger_moneypile::DoParticleBurst()
{
	if (IsHidden())
	{
		return;
	}

	gameLocal.DoParticle(spawnArgs.GetString("model_break"), GetPhysics()->GetOrigin(), idVec3(-90,0,0), true, false, spawnArgs.GetVector("_color", "1 1 1"));
	StartSound("snd_break", SND_CHANNEL_ANY);

	Hide();
	PostEventMS(&EV_Remove, 0);	
}