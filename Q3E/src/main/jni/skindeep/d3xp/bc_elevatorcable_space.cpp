#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_frobcube.h"
#include "bc_elevatorcable_space.h"

//TODO: attach/show a climbing device

const int FROBTRIGGER_WIDTH = 2;
const int ENDPOINT_PROXIMITY_THRESHOLD = 192; //WHEn you're this close to an endpoint, we will assume you don't want to zip to it. So, zip the player the other direction.



CLASS_DECLARATION(idBeam, idElevatorcableSpace)
	EVENT(EV_PostSpawn,	idElevatorcableSpace::Event_PostSpawn)
END_CLASS

idElevatorcableSpace::idElevatorcableSpace()
{
	frobTrigger = NULL;
	lastZipDirection = 0;
}

idElevatorcableSpace::~idElevatorcableSpace()
{
	if (frobTrigger)
	{
		frobTrigger->PostEventMS(&EV_Remove, 0);
		frobTrigger = nullptr;
	}
}

void idElevatorcableSpace::Spawn(void)
{
}

void idElevatorcableSpace::Event_PostSpawn(void)
{
	const char *topName;
	const char *bottomName;

	topName = spawnArgs.GetString("target_top", "");
	if (*topName == '\0')
	{
		gameLocal.Error("ElevatorCableSpace %s has invalid target_top value.", this->GetName());
	}
	else
	{
		topEnt = gameLocal.FindEntity(topName);

		if (!topEnt.IsValid())
		{
			gameLocal.Error("ElevatorCableSpace %s couldn't find target_top: %s", this->GetName(), topName);
		}

		//gameRenderWorld->DebugSphere(colorGreen, idSphere(topEnt->GetPhysics()->GetOrigin(), 4), 10000);
	}

	bottomName = spawnArgs.GetString("target_bottom", "");
	if (*bottomName == '\0')
	{
		gameLocal.Error("ElevatorCableSpace '%s' has invalid target_bottom value.", this->GetName());
	}
	else
	{
		bottomEnt = gameLocal.FindEntity(bottomName);

		if (!bottomEnt.IsValid())
		{
			gameLocal.Error("ElevatorCableSpace '%s' couldn't find target_bottom: %s", this->GetName(), bottomName);
		}
	}

	idBeam::Event_MatchTarget();

	if (!target.IsValid())
	{
		gameLocal.Error("ElevatorCableSpace '%s' has no target.", GetName());
	}

	float lengthOfBeam = (target.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).Length();

	//Spawn the frob volume.	
	idDict args;
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetVector("mins", idVec3(-FROBTRIGGER_WIDTH, -FROBTRIGGER_WIDTH, 0));
	args.SetVector("maxs", idVec3(FROBTRIGGER_WIDTH, FROBTRIGGER_WIDTH, lengthOfBeam));
	args.SetBool("frobcable_space", true);
	args.Set("displayname", "#str_def_gameplay_100022"); //BC 3-20-2025: fixed loc bug
	frobTrigger = (idFrobcube *)gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	//common->Printf("%s\n", frobTrigger->GetName());	
	//frobTrigger->GetPhysics()->SetContents(CONTENTS_CORPSE);
	//frobTrigger->isFrobbable = true;

	if (frobTrigger)
	{
		idAngles beamDir = (target.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).ToAngles();
		beamDir.pitch += 90;
		frobTrigger->SetAxis(beamDir.ToMat3());
		frobTrigger->GetPhysics()->GetClipModel()->SetOwner(this);
	}
	else
	{
		gameLocal.Error("failed to spawn frobTrigger for '%s'", GetName());
	}



	//Spawn the little plug models.
	for (int i = 0; i < 2; i++)
	{
		idVec3 plugPos;
		idMat3 plugAngle;
		
		if (i == 0)
		{
			//start point.
			plugPos = GetPhysics()->GetOrigin();
			idVec3 cableDirection = target.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			cableDirection.Normalize();
			plugAngle = cableDirection.ToMat3();
		}
		else
		{
			//end point.
			plugPos = target.GetEntity()->GetPhysics()->GetOrigin();
			idVec3 cableDirection = GetPhysics()->GetOrigin() - target.GetEntity()->GetPhysics()->GetOrigin();
			cableDirection.Normalize();
			plugAngle = cableDirection.ToMat3();
		}
		

		args.Clear();
		args.SetInt("solid", 0);
		args.Set("model", "models/objects/elevatorplug/spaceplug_4x4x8.ase");
		args.SetVector("origin", plugPos);
		args.SetMatrix("rotation", plugAngle);
		args.Set("_color", "0 1 1");
		(idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	}


	BecomeActive(TH_THINK);
}



void idElevatorcableSpace::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( topEnt ); //  idEntityPtr<idEntity> topEnt
	savefile->WriteObject( bottomEnt ); //  idEntityPtr<idEntity> bottomEnt

	savefile->WriteObject( frobTrigger ); //  idFrobcube* frobTrigger

	savefile->WriteInt( lastZipDirection ); //  int lastZipDirection
}

void idElevatorcableSpace::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( topEnt ); //  idEntityPtr<idEntity> topEnt
	savefile->ReadObject( bottomEnt ); //  idEntityPtr<idEntity> bottomEnt

	savefile->ReadObject( CastClassPtrRef(frobTrigger) ); //  idFrobcube* frobTrigger

	savefile->ReadInt( lastZipDirection ); //  int lastZipDirection
}

void idElevatorcableSpace::Think(void)
{
	idBeam::Think();

	if (!gameLocal.InPlayerPVS(this)) //if player can't see me, then skip all this.
	{
		if (lastZipDirection != DIR_NONE)
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
			lastZipDirection = DIR_NONE;
		}
		return;
	}

	if (!gameLocal.GetLocalPlayer()->GetSwoopState())
	{
		if (!frobTrigger->isFrobbable)
		{
			frobTrigger->isFrobbable = true;
		}
	}

	if (gameLocal.GetLocalPlayer()->GetFrobEnt() == NULL)
	{
		if (lastZipDirection != DIR_NONE)
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
			lastZipDirection = DIR_NONE;
		}
		return;
	}

	if (gameLocal.GetLocalPlayer()->GetFrobEnt()->entityNumber != frobTrigger->entityNumber)
	{
		if (lastZipDirection != DIR_NONE)
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
			lastZipDirection = DIR_NONE;
		}
		return;
	}

	//Ok, player is looking at me.
	int directionToZip = GetZipDirection();
	if (lastZipDirection != directionToZip)
	{
		if (directionToZip == DIR_TO_START)
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_left")));
		}
		else if (directionToZip == DIR_TO_END)
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_right")));
		}

		lastZipDirection = directionToZip;
	}
}

//This is the logic that determines what direction we will zip the player. This is dependent on:
// - what direction the player is looking
// - whether the player can see the startpoint and/or the endpoint
// - whether the player is very close to a startpoint or endpoint.
int idElevatorcableSpace::GetZipDirection()
{
	idVec3 playerDir = gameLocal.GetLocalPlayer()->viewAngles.ToForward();

	idVec3 dirToStart = topEnt.GetEntity()->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
	dirToStart.Normalize();
	float vdotToStart = DotProduct(dirToStart, playerDir);

	idVec3 dirToEnd = bottomEnt.GetEntity()->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
	dirToEnd.Normalize();
	float vdotToEnd = DotProduct(dirToEnd, playerDir);

	float distanceToStart = (topEnt.GetEntity()->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin).LengthFast();
	float distanceToEnd = (bottomEnt.GetEntity()->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin).LengthFast();

	if (distanceToStart < ENDPOINT_PROXIMITY_THRESHOLD)
		return DIR_TO_END;
	else if (distanceToEnd < ENDPOINT_PROXIMITY_THRESHOLD)
		return DIR_TO_START;

	if ((vdotToStart < 0 && vdotToEnd < 0) || (vdotToStart > 0 && vdotToEnd > 0))
	{
		//If player can see both the START and the END in their fov (or, can see neither of them), then we do a distance check.

		//Find the farther point.
		if (distanceToStart > distanceToEnd)
		{
			//The start point is farther. Go to the start point.
			return DIR_TO_START;
		}
		else
		{
			//End point is farther. Go to end point.
			return DIR_TO_END;
		}
	}

	//See whether START or END is in player's fov.
	if (vdotToStart > vdotToEnd)
	{
		//Can see start (and can't see end). Head to the start.
		return DIR_TO_START;
	}	

	//Can see end (and can't see start). Head to the end.
	return DIR_TO_END;
}

bool idElevatorcableSpace::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	if (gameLocal.GetLocalPlayer()->GetSwoopState()) //Don't allow swoop if already swooping.
		return false;


	gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);

	frobTrigger->isFrobbable = false;

	if (lastZipDirection == DIR_TO_END)
	{
		gameLocal.GetLocalPlayer()->SpaceCableswoop(topEnt.GetEntity(), bottomEnt.GetEntity());
	}
	else if (lastZipDirection == DIR_TO_START)
	{
		gameLocal.GetLocalPlayer()->SpaceCableswoop(bottomEnt.GetEntity(), topEnt.GetEntity());
	}

	return true;
}


