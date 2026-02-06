//#include "sys/platform.h"
//#include "Entity.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "Item.h"
#include "bc_nutrienttube.h"

const int DISPENSETIME = 500;

CLASS_DECLARATION(idStaticEntity, idNutrientTube)
END_CLASS

idNutrientTube::idNutrientTube(void)
{
	state = NT_IDLE;
}

idNutrientTube::~idNutrientTube(void)
{
}

void idNutrientTube::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
}

void idNutrientTube::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
}

void idNutrientTube::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
}

void idNutrientTube::Think(void)
{	
	if (state == NT_DISPENSING)
	{
		if (gameLocal.time >= stateTimer + DISPENSETIME)
		{
			isFrobbable = true;
			BecomeInactive(TH_THINK);
			state = NT_IDLE;
		}		
	}	

	idStaticEntity::Think();
}



bool idNutrientTube::DoFrob(int index, idEntity * frobber)
{
	if (state == NT_IDLE)
	{
		isFrobbable = false;
		state = NT_DISPENSING;
		BecomeActive(TH_THINK);

		idVec3 forward, right, up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		idVec3 squirtOffset = spawnArgs.GetVector("spigotoffset");
		idVec3 squirtPos = GetPhysics()->GetOrigin() + (forward * squirtOffset.x) + (right * squirtOffset.y)  + (up * squirtOffset.z);

		idAngles particleAng;
		particleAng = GetPhysics()->GetAxis().ToAngles();
		particleAng.pitch += 179;
		idEntity *particle = gameLocal.DoParticleAng(spawnArgs.GetString("model_squirt"), squirtPos, particleAng);
		if (particle)
		{

			particle->SetColor(spawnArgs.GetVector("_color"));
		}

		StartSound("snd_squirt", SND_CHANNEL_ANY);
		stateTimer = gameLocal.time;
	}
	
	return true;
}
