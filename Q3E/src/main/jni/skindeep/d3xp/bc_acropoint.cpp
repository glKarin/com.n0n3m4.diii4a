#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_acropoint.h"

CLASS_DECLARATION(idEntity, idAcroPoint)
END_CLASS

void idAcroPoint::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(state);  //  int state
	savefile->WriteFloat(baseAngle); //  float baseAngle
}

void idAcroPoint::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt(state);  // int state
	savefile->ReadFloat(baseAngle); //  float baseAngle
}


void idAcroPoint::Spawn(void)
{
	state = spawnArgs.GetInt("acrotype");
	GetPhysics()->SetContents(CONTENTS_CORPSE);

	baseAngle = GetPhysics()->GetAxis().ToAngles().yaw;
}

bool idAcroPoint::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	idAngles acroBaseAngle, playerDir;
	float vdot;

	isFrobbable = false; //turn off this acropoint's frobbability.

	//Determine whether we need to flip the acropoint 180 degrees depending on player's direction.
	playerDir = gameLocal.GetLocalPlayer()->viewAngles;
	playerDir.pitch = 0;
	playerDir.roll = 0;

	acroBaseAngle = idAngles(0, this->baseAngle, 0);

	if (spawnArgs.GetBool("autodir", "1"))
	{
		vdot = DotProduct(acroBaseAngle.ToForward(), playerDir.ToForward());

		if (vdot < 0)
		{
			//if player is approaching the acropoint from its front, then flip it.
			idAngles flippedAngle = idAngles(0, acroBaseAngle.yaw + 180.0f, 0);
			this->SetAngles(flippedAngle);
		}
		else
		{
			this->SetAngles(acroBaseAngle);
		}
	}

	

	//StartSound("snd_enter", SND_CHANNEL_ANY, 0, false, NULL);


	gameLocal.GetLocalPlayer()->wasCaughtEnteringCargoHide = gameLocal.GetLocalPlayer()->IsCurrentlySeenByAI();
	if (gameLocal.GetLocalPlayer()->wasCaughtEnteringCargoHide)
	{
		gameLocal.AddEventLog("#str_def_gameplay_caughthiding", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin());
	}

	gameLocal.GetLocalPlayer()->SetAcroHide(this); //Tell physics system to move player into the hidey spot.

	return true;
}



//void idAcroPoint::Present(void)
//{
//	// don't present to the renderer if the entity hasn't changed
//	if (!(thinkFlags & TH_UPDATEVISUALS))
//	{
//		return;
//	}
//
//	BecomeInactive(TH_UPDATEVISUALS);
//
//	// if set to invisible, skip
//	if (!renderEntity.hModel || IsHidden())
//	{
//		return;
//	}
//
//	// add to refresh list
//	if (modelDefHandle == -1)
//	{
//		modelDefHandle = gameRenderWorld->AddEntityDef(&renderEntity);
//	}
//	else
//	{
//		gameRenderWorld->UpdateEntityDef(modelDefHandle, &renderEntity);
//	}
//}
