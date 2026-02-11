#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_elevatorcable.h"

//TODO: attach/show a climbing device

const int ANIMBUFFERTIME = 100;

const int DISTANCETHRESHOLD = 64;

CLASS_DECLARATION(idAnimated, idElevatorcable)
END_CLASS


void idElevatorcable::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_CORPSE);
	this->isFrobbable = true;

	hasInitialized = false;	
	animState = ELEVCABLESTATE_IDLE;
}

void idElevatorcable::Initialize()
{
	const char *topName;
	const char *bottomName;

	topName = spawnArgs.GetString("target_top", "");
	if (*topName == '\0')
	{
		gameLocal.Error("ElevatorCable %s has invalid target_top value.", this->GetName());
	}
	else
	{
		topEnt = gameLocal.FindEntity(topName);

		if (!topEnt)
		{
			gameLocal.Error("ElevatorCable %s couldn't find target_top: %s", this->GetName(), topName);
		}

		//gameRenderWorld->DebugSphere(colorGreen, idSphere(topEnt->GetPhysics()->GetOrigin(), 4), 10000);
	}

	bottomName = spawnArgs.GetString("target_bottom", "");
	if (*bottomName == '\0')
	{
		gameLocal.Error("ElevatorCable %s has invalid target_bottom value.", this->GetName());
	}
	else
	{
		bottomEnt = gameLocal.FindEntity(bottomName);

		if (!bottomEnt)
		{
			gameLocal.Error("ElevatorCable %s couldn't find target_bottom: %s", this->GetName(), bottomName);
		}

		//gameRenderWorld->DebugSphere(colorRed, idSphere(bottomEnt->GetPhysics()->GetOrigin(), 4), 10000);
	}

	this->Event_PlayAnim("idle", 0, true);
}

void idElevatorcable::Think(void)
{
	idAnimated::Think();

	if (!hasInitialized)
	{
		hasInitialized = true;
		Initialize();
	}

	if (animState == ELEVCABLESTATE_WAITING)
	{
		if (gameLocal.time >= animTimer + 150 + 900 + ANIMBUFFERTIME) //swoop values from physics_player: time to move to cable + time to move up cable + buffer time
		{
			animTimer = gameLocal.time;
			this->Event_PlayAnim("jostle", 0, false);
			animState = ELEVCABLESTATE_WIGGLING;
		}
	}
	else if (animState == ELEVCABLESTATE_WIGGLING)
	{
		if (gameLocal.time >= animTimer + 1900) //how long jostle anim is.
		{
			animState = ELEVCABLESTATE_IDLE;
			this->Event_PlayAnim("idle", 8, true);
		}
	}
	
}

void idElevatorcable::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( hasInitialized ); //  bool hasInitialized

	savefile->WriteObject( topEnt ); //  idEntity * topEnt
	savefile->WriteObject( bottomEnt ); //  idEntity * bottomEnt

	savefile->WriteInt( animTimer ); //  int animTimer
	savefile->WriteInt( animState ); //  int animState
}

void idElevatorcable::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( hasInitialized ); //  bool hasInitialized

	savefile->ReadObject( topEnt ); //  idEntity * topEnt
	savefile->ReadObject( bottomEnt ); //  idEntity * bottomEnt

	savefile->ReadInt( animTimer ); //  int animTimer
	savefile->ReadInt( animState ); //  int animState
}

bool idElevatorcable::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	float distanceToTop;

	if (gameLocal.GetLocalPlayer()->GetSwoopState()) //Don't allow swoop if already swooping.
		return false;

	distanceToTop = (topEnt->GetPhysics()->GetOrigin() - idVec3(topEnt->GetPhysics()->GetOrigin().x,topEnt->GetPhysics()->GetOrigin().y,gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z)).LengthFast();
	//distanceToBottom = (bottomEnt->GetPhysics()->GetOrigin() - idVec3(bottomEnt->GetPhysics()->GetOrigin().x, bottomEnt->GetPhysics()->GetOrigin().y, gameLocal.GetLocalPlayer()->GetEyePosition().z)).LengthFast();

	gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);

	//If player is close to top (or above the top), then slide downward.
	if (distanceToTop <= DISTANCETHRESHOLD || gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z > topEnt->GetPhysics()->GetOrigin().z)
	{
		//Slide down.
		gameLocal.GetLocalPlayer()->Cableswoop(topEnt, bottomEnt);
	}
	else
	{
		//All other cases: slide upward.
		gameLocal.GetLocalPlayer()->Cableswoop(bottomEnt, topEnt);		
	}

	
	this->Event_PlayAnim("idle", 0, true);
	animState = ELEVCABLESTATE_WAITING;
	animTimer = gameLocal.time;

	return true;
}


