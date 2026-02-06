#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"


#include "bc_skullsaver.h"
#include "bc_escapepod.h"

const int PAUSETIME = 800;
const int SINKTIME = 200;
const int FORWARD_DISTANCE = 8;


CLASS_DECLARATION(idAnimated, idEscapePod)
END_CLASS

idEscapePod::idEscapePod(void)
{
}

idEscapePod::~idEscapePod(void)
{
}

void idEscapePod::Spawn(void)
{
	stateTimer = 0;
	state = IDLE;
	isFrobbable = true;
	fl.takedamage = false;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	skullPtr = NULL;
	BecomeInactive(TH_THINK);

	//BC test: disable escape pods
	this->Hide();
}

void idEscapePod::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( stateTimer ); //  int stateTimer
	savefile->WriteInt( state ); //  int state

	savefile->WriteObject( skullPtr ); //  idEntityPtr<idEntity> skullPtr
	savefile->WriteVec3( skullStartPos ); //  idVec3 skullStartPos
	savefile->WriteVec3( skullEndPos ); //  idVec3 skullEndPos
}

void idEscapePod::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( stateTimer ); //  int stateTimer
	savefile->ReadInt( state ); //  int state

	savefile->ReadObject( skullPtr ); //  idEntityPtr<idEntity> skullPtr
	savefile->ReadVec3( skullStartPos ); //  idVec3 skullStartPos
	savefile->ReadVec3( skullEndPos ); //  idVec3 skullEndPos
}


void idEscapePod::Think(void)
{
	if (state == INITIALPAUSE)
	{
		if (skullPtr.IsValid())
		{
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			skullPtr.GetEntity()->SetOrigin(GetPhysics()->GetOrigin() + forward * FORWARD_DISTANCE);
		}

		if (gameLocal.time >= stateTimer)
		{
			state = SINK_INSIDE;
			stateTimer = gameLocal.time + SINKTIME;

			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			skullStartPos = skullPtr.GetEntity()->GetPhysics()->GetOrigin();
			skullEndPos = this->GetPhysics()->GetOrigin() + forward * -8;
		}
	}
	else if (state == SINK_INSIDE)
	{
		float lerp = 1.0f - ((stateTimer - gameLocal.time) / (float)SINKTIME);
		lerp = idMath::ClampFloat(0, 1, lerp);

		idVec3 lerpedPos;
		lerpedPos.Lerp(skullStartPos, skullEndPos, lerp);
		skullPtr.GetEntity()->SetOrigin(lerpedPos);

		if (gameLocal.time >= stateTimer)
		{
			state = DONE;
			skullPtr.GetEntity()->Hide();
			skullPtr.GetEntity()->PostEventMS(&EV_Remove, 0);

			//Store the skull.
			if (skullPtr.GetEntity()->IsType(idSkullsaver::Type))
			{
				static_cast<idSkullsaver*>(skullPtr.GetEntity())->StoreSkull();
			}

			SetColor(.3f, .3f, .3f);
			UpdateVisuals();
		}
	}

	idAnimated::Think();
}

bool idEscapePod::DoFrob(int index, idEntity * frobber)
{
	if (state != IDLE || IsHidden())
		return false;

	if (frobber == gameLocal.GetLocalPlayer())
	{
		//Check if player is holding onto a skullsaver.
		if (gameLocal.GetLocalPlayer()->HasWeaponInInventory("weapon_skullsaver"))
		{
			//Player has skullsaver in inventory.

			idEntity *skull = gameLocal.GetLocalPlayer()->GetCarryableFromInventory("weapon_skullsaver");
			if (skull != NULL)
			{
				if (gameLocal.GetLocalPlayer()->RemoveCarryableFromInventory("weapon_skullsaver"))
				{
					StartDeploymentSequence(skull);
				}
			}
			return true;
		}
		else
		{
			//Player does NOT have skull in inventory.
			//gameLocal.GetLocalPlayer()->SetCenterMessage("You don't have a skullsaver.");
			return true;
		}
	}
	else if (frobber->IsType(idSkullsaver::Type))
	{
		StartDeploymentSequence(frobber);
		return true;
	}

	return true;
}

void idEscapePod::StartDeploymentSequence(idEntity *ent)
{
	skullPtr = ent;
	if (!skullPtr.IsValid())
	{
		return;
	}

	isFrobbable = false;
	state = INITIALPAUSE;	
	stateTimer = gameLocal.time + PAUSETIME;
	ent->isFrobbable = false;
	ent->SetAxis(GetPhysics()->GetAxis());
	if (ent->IsType(idSkullsaver::Type))
	{
		static_cast<idSkullsaver*>(ent)->SetInEscapePod();
	}
	BecomeActive(TH_THINK);
	
	//POsition the head.
	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);		
	ent->SetOrigin(GetPhysics()->GetOrigin() + forward * FORWARD_DISTANCE);

	if (skullPtr.GetEntity()->IsType(idSkullsaver::Type))
	{
		if (static_cast<idSkullsaver *>(skullPtr.GetEntity())->bodyOwner.IsValid())
		{
			gameLocal.GetLocalPlayer()->DoEliminationMessage(static_cast<idSkullsaver *>(skullPtr.GetEntity())->bodyOwner.GetEntity()->displayName.c_str(), false);
		}
	}
}