#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "bc_cargohide.h"



CLASS_DECLARATION(idStaticEntity, idCargohide)

END_CLASS


idCargohide::idCargohide(void)
{
}

idCargohide::~idCargohide(void)
{
}

void idCargohide::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	isFrobbable = true;
}

void idCargohide::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( maxhealth ); //  int maxhealth
}

void idCargohide::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( maxhealth ); //  int maxhealth
}

void idCargohide::Think(void)
{
	idStaticEntity::Think();
}

void idCargohide::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
}

bool idCargohide::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	//Determine whether someone saw me entering this cargo hide.
	gameLocal.GetLocalPlayer()->wasCaughtEnteringCargoHide = gameLocal.GetLocalPlayer()->IsCurrentlySeenByAI();
	if (gameLocal.GetLocalPlayer()->wasCaughtEnteringCargoHide)
		gameLocal.AddEventLog("#str_def_gameplay_caughthiding", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin());


	isFrobbable = false;
	StartSound("snd_enter", SND_CHANNEL_ANY, 0, false, NULL);
	gameLocal.GetLocalPlayer()->SetHideState(this, spawnArgs.GetInt("cargotype", "0")); //Tell physics system to move player into the hidey spot.
	return true;
}

