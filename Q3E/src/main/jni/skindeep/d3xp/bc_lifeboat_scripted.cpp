//#include "sys/platform.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "framework/DeclEntityDef.h"

//#include "ai\AI.h"
//#include "bc_gunner.h"
#include "bc_lifeboat_scripted.h"
#include "bc_lifeboat.h"

CLASS_DECLARATION(idEntity, idLifeboatScripted)
	EVENT(EV_Activate, idLifeboatScripted::Event_Activate)
END_CLASS



#define DOTTHRESHOLD .7f //how closely the player has to look at the pod

#define LOOKCOUNTTHRESHOLD 10 //Time duration "ticks" the player has to look to activate the pod launch.

#define DISTANCETHRESHOLD 160



idLifeboatScripted::~idLifeboatScripted(void)
{
	if (beamStart.IsValid())
	{
		beamStart.GetEntity()->PostEventMS(&EV_Remove, 0);
	}

	if (beamEnd.IsValid())
	{
		beamEnd.GetEntity()->PostEventMS(&EV_Remove, 0);
	}
}

idLifeboatScripted::idLifeboatScripted(void)
{
	state = LFS_NONE;
	stateTimer = 0;
	lookCounter = 0;

	beamStart = NULL;
	beamEnd = NULL;

}

void idLifeboatScripted::Spawn(void)
{
	BecomeInactive(TH_THINK);
	PostEventMS(&EV_PostSpawn, 0);	
}

void idLifeboatScripted::Event_PostSpawn(void)
{
	if (targets.Num() <= 0)
	{
		gameLocal.Error("target_lifeboat_scripted '%s' is missing target", GetName());
		return;
	}

	//debug. make it activate at level start.
	//Event_Activate(nullptr);
}

void idLifeboatScripted::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( lookCounter ); // int lookCounter

	savefile->WriteObject( beamEnd ); // idEntity* beamEnd
	savefile->WriteObject( beamStart ); // idEntity* beamStart
}

void idLifeboatScripted::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state

	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( lookCounter ); // int lookCounter

	savefile->ReadObject( beamEnd ); // idEntity* beamEnd
	savefile->ReadObject( beamStart ); // idEntity* beamStart
}

void idLifeboatScripted::Think(void)
{
	if (state == LFS_WAITINGFORPLAYER)
	{
		UpdateWaitingForPlayer();

		if (lookCounter >= LOOKCOUNTTHRESHOLD)
		{
			LaunchPod();
		}
	}
}

void idLifeboatScripted::LaunchPod()
{
	if (state == LFS_ACTIVE)
		return;

	state = LFS_ACTIVE;

	StartSound("snd_locked", SND_CHANNEL_ANY);	

	const idDeclEntityDef* podDef;
	podDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_pod"),  false);
	if (podDef == nullptr)
	{
		gameLocal.Error("idLifeboatScripted: failed to find pod def.");
	}

	idEntity* podEnt = NULL;
	gameLocal.SpawnEntityDef(podDef->dict, &podEnt, false);
	if (podEnt == nullptr)
	{
		gameLocal.Error("idLifeboatScripted: failed to spawn pod ent.");
	}

	//Launch it.
	idVec3 dirToTarget;
	idVec3 spawnPosition;
	spawnPosition = GetPhysics()->GetOrigin();
	podEnt->SetOrigin(spawnPosition);

	//Set pod orientation.
	idVec3 targetPos = targets[0].GetEntity()->GetPhysics()->GetOrigin();
	dirToTarget = targetPos - spawnPosition;
	dirToTarget.Normalize();
	podEnt->SetAxis(dirToTarget.ToMat3());

	static_cast<idLifeboat*>(podEnt)->Launch(targetPos);

	
	PostEventMS(&EV_Remove, 0);
}

void idLifeboatScripted::UpdateWaitingForPlayer()
{
	if (gameLocal.time < stateTimer)
		return;

	stateTimer = gameLocal.time + 100;

	//get distance to the target position.
	idVec3 targetPos = targets[0].GetEntity()->GetPhysics()->GetOrigin();
	float playerDistance = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - targetPos).Length();
	if (playerDistance <= DISTANCETHRESHOLD)
	{
		lookCounter = 9999999; //if player is close, just launch it.
		return;
	}

	//detect if player has line of sight, or is close enough.	
	if (IsInPlayerLOS(GetPhysics()->GetOrigin()) || IsInPlayerLOS(targetPos))
	{
		lookCounter++;
	}
	else
	{
		lookCounter = 0;
	}	
}

bool idLifeboatScripted::IsInPlayerLOS(idVec3 _pos)
{
	trace_t tr;
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, _pos, MASK_SOLID, NULL);
	if (tr.fraction < 1)
		return false;

	idVec3 dirToEnt = _pos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
	dirToEnt.Normalize();
	float facingResult = DotProduct(dirToEnt, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
	if (facingResult >= DOTTHRESHOLD)
		return true;

	return false;
}

void idLifeboatScripted::Event_Activate(idEntity* activator)
{
	if (state != LFS_NONE)
		return;

	state = LFS_WAITINGFORPLAYER;
	BecomeActive(TH_THINK);

	idVec3 targetPos = targets[0].GetEntity()->GetPhysics()->GetOrigin();
	idDict args;
	beamEnd = (idBeam*)gameLocal.SpawnEntityType(idBeam::Type, &args);
	beamEnd.GetEntity()->SetOrigin(targetPos);

	args.Clear();
	args.Set("target", beamEnd.GetEntity()->name.c_str());
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetBool("start_off", false);
	args.Set("width", spawnArgs.GetString("laserwidth", "8"));
	args.Set("skin", spawnArgs.GetString("laserskin", "skins/beam_beacon"));
	beamStart = (idBeam*)gameLocal.SpawnEntityType(idBeam::Type, &args);
	beamStart.GetEntity()->Hide(); //For some reason start_off causes problems.... so spawn it normally, and then do a hide() here. oh well :/
	
}