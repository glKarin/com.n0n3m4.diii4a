#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"
#include "framework/DeclEntityDef.h"

#include "bc_trashchute.h"

#include "bc_ventpeek.h"
#include "bc_trashexit.h"


CLASS_DECLARATION(idStaticEntity, idTrashExit)
	EVENT(EV_SpectatorTouch, idTrashExit::Event_SpectatorTouch)
END_CLASS



idTrashExit::idTrashExit(void)
{
	spacenudgeNode.SetOwner(this);
	spacenudgeNode.AddToEnd(gameLocal.spacenudgeEntities);

	gameLocal.trashexitEntities.Append(this);

	peekEnt = nullptr;
	gateModel = nullptr;
}

idTrashExit::~idTrashExit(void)
{
	spacenudgeNode.Remove();
}

void idTrashExit::Spawn(void)
{
	//GetPhysics()->SetContents(0);

	isFrobbable = true;

	//lockdown gate.
	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idDict args;
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin() + (forward * 2.1f));
	args.SetMatrix("rotation", GetPhysics()->GetAxis());
	args.Set("model", spawnArgs.GetString("model_gate"));
	args.SetInt("solid", 0);
	args.SetBool("noclipmodel", true);
	gateModel = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	gateModel->Hide();

	if (!spawnArgs.GetBool("start_on", "1"))
	{
		Event_ChuteEnable(0);
	}
}



void idTrashExit::SetupChute(idEntity * ent, bool isOpen)
{
	myChute = ent;

	//Spawn the ventpeek stuff.
	if (spawnArgs.GetBool("has_peek", "1"))
	{
		idDict args;

		//Spawn ventpeek target. This is where the player peeks OUT of (the chute entrance).
		idEntity *nullEnt;
		idVec3 chuteForward, chuteUp;
		ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&chuteForward, NULL, &chuteUp);
		idVec3 chutePos = ent->GetPhysics()->GetOrigin() + (chuteUp * 50) + (chuteForward * -8);
		args.SetVector("origin", chutePos);
		args.SetMatrix("rotation", ent->GetPhysics()->GetAxis());
		args.Set("classname", "target_null");
		gameLocal.SpawnEntityDef(args, &nullEnt);

		//spawn a ventpeek. This is where the peek peeks INTO (the chute exit).
		idVec3 forwardDir, rightDir;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);
		idVec3 peekPos = this->GetPhysics()->GetOrigin() + (forwardDir * 2.0f) + (rightDir * 9.5f);
		idAngles finalAng = this->GetPhysics()->GetAxis().ToAngles();
		idEntity * tempEnt;
		args.Clear();
		args.Set("classname", "env_ventpeek_ventdoor");
		args.SetVector("origin", peekPos);
		args.SetMatrix("rotation", finalAng.ToMat3());
		args.Set("target0", nullEnt->GetName());
		args.SetFloat("cam_offset", 0.0f); // this is already offset up above on chutePos
		args.SetBool("spawnpeek", false);
		args.SetBool("use_targetangle", true);
		//args.Set("model", "models/objects/ventpeek/ventpeekmini_8.ase");
		gameLocal.SpawnEntityDef(args, &tempEnt);
		if (!tempEnt)
		{
			gameLocal.Error("ventdoor '%s' failed to spawn ventpeek.");
		}
		peekEnt = static_cast<idVentpeek*>(tempEnt);
		
		peekEnt->forVentDoor = true; //This is to make the ventpeek not be frobbable after the player completes a peek.


		peekEnt->isFrobbable = false; //We only want this accessible via the hold-frob.
		peekEnt->GetPhysics()->SetContents(0);
	}
	else
	{
		peekEnt = NULL;
	}

	Event_ChuteEnable(isOpen);
}

void idTrashExit::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( myChute ); // idEntityPtr<idEntity> myChute
	savefile->WriteObject( peekEnt ); // class idVentpeek * peekEnt
	savefile->WriteObject( gateModel ); // idEntity * gateModel
}

void idTrashExit::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( myChute ); // idEntityPtr<idEntity> myChute
	savefile->ReadObject( CastClassPtrRef(peekEnt) ); // class idVentpeek * peekEnt
	savefile->ReadObject( gateModel ); // idEntity * gateModel
}


bool idTrashExit::DoFrob(int index, idEntity * frobber)
{
	//BC 5-8-2025: only player can frob the trash exit.
	if (frobber == nullptr)
	{
		return false;
	}

	if (frobber != gameLocal.GetLocalPlayer())
	{
		return false;
	}


	if (!myChute.IsValid())
	{
		gameLocal.Error("trash exit '%s' has invalid chute.", this->GetName());
		return false;
	}


	//see if trash chute is currently locked/unlocked.

	if (myChute.GetEntity()->IsType(idTrashchute::Type))
	{
		bool chuteEnabled = static_cast<idTrashchute *>(myChute.GetEntity())->IsChuteEnabled();

		if (!chuteEnabled)
		{
			StartSound("snd_error", SND_CHANNEL_ANY, 0, false, NULL);
			return false;
		}
	}


	idVec3 candidateSpot = FindClearExitPosition();
	if (candidateSpot != vec3_zero)
	{
		DoPlayerTeleport(candidateSpot);
		return true;
	}

	if (1)
	{
		//Failed to find spot. Attempt to teleport player directly on top of chute.
		trace_t teleTr;
		idVec3 candidateSpot;

		idVec3 forward;
		idBounds playerbounds;
		playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
		myChute.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

		candidateSpot = myChute.GetEntity()->GetPhysics()->GetOrigin() + idVec3(0, 0, 49.0f) + (forward * 1.0f);

		gameLocal.clip.TraceBounds(teleTr, candidateSpot, candidateSpot, playerbounds, MASK_SOLID, NULL);

		if (teleTr.fraction >= 1.0f)
		{
			//TODO: add a "cooldown" delay so that the player doesn't immediately get sucked back into trashchute after exiting from it.
			DoPlayerTeleport(candidateSpot);
			return true;
		}
	}


	//was UNABLE to find a safe spot in front of chute....
	StartSound("snd_error", SND_CHANNEL_ANY, 0, false, NULL);
	return false;
}

bool idTrashExit::DoFrobHold(int index, idEntity * frobber)
{
	bool result = false;

	if (peekEnt)
	{
		result = peekEnt->DoFrob(index, frobber);
	}

	return result;
}

idVec3 idTrashExit::FindClearExitPosition()
{
	//Attempt to find a safe spot to teleport player to.
	if (!myChute.IsValid())
	{
		return vec3_zero;
	}

	idVec3 forward, right;
	int distanceArray[] = { 0, 16, -16, 32, -32, 48, -48 };
	idBounds playerbounds;
	
	playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	// When spectating the z is negative which messes up the trace
	if (gameLocal.GetLocalPlayer()->spectating) {
		playerbounds[0].z = 0.0f;
	}
	myChute.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, NULL);

	for (int i = 0; i < 7; i++)
	{
		idVec3 candidateSpot;
		trace_t teleTr;

		candidateSpot = myChute.GetEntity()->GetPhysics()->GetOrigin() + (forward * 33) + (right * distanceArray[i]) + idVec3(0, 0, 2.5f);
		gameLocal.clip.TraceBounds(teleTr, candidateSpot, candidateSpot, playerbounds, MASK_SOLID, NULL);

		if (teleTr.fraction >= 1.0f)
		{
			// When spectating, need to set the desired z higher
			if ( gameLocal.GetLocalPlayer()->spectating ) {
				candidateSpot.z += 48.0f;
			}
			return candidateSpot;
		}
	}

	return vec3_zero;
}



void idTrashExit::DoPlayerTeleport(idVec3 position)
{
	idAngles machineAng;

	machineAng = myChute.GetEntity()->GetPhysics()->GetAxis().ToAngles();

	gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 0.0f, 300);
	gameLocal.GetLocalPlayer()->Teleport(position, machineAng, NULL, false, false);
	gameLocal.GetLocalPlayer()->SetSmelly(true);

	myChute.GetEntity()->StartSound("snd_emerge", SND_CHANNEL_ANY, 0, false, NULL);
}

void idTrashExit::Event_SpectatorTouch(idEntity *other, trace_t *trace)
{
	idVec3		contact, translate, normal;
	idBounds	bounds;
	idPlayer	*p;

	assert(other && other->IsType(idPlayer::Type) && static_cast<idPlayer *>(other)->spectating);

	p = static_cast<idPlayer *>(other);
	// avoid flicker when stopping right at clip box boundaries
	if (p->lastSpectateTeleport > gameLocal.hudTime - 300) { //BC was 1000
		return;
	}
	
	idVec3 candidateSpot = FindClearExitPosition();
	if (candidateSpot == vec3_zero)
	{
		return;
	}

	p->SetOrigin(candidateSpot);
	p->SetViewAngles(myChute.GetEntity()->GetPhysics()->GetAxis().ToAngles());
	p->lastSpectateTeleport = gameLocal.hudTime;
	gameLocal.GetLocalPlayer()->StartSound("snd_spectate_door", SND_CHANNEL_ANY);
}

//Lockdown control. 0 = lock down, 1 = is unlocked/open
void idTrashExit::Event_ChuteEnable(int value)
{
	if (value <= 0)
	{
		//lock it down.
		isFrobbable = false;
		gateModel->Show();
	}
	else
	{
		//release lockdown.
		isFrobbable = true;
		gateModel->Hide();
	}
}

bool idTrashExit::IsFuseboxTrashgateShut()
{
	return !gateModel->IsHidden();
}