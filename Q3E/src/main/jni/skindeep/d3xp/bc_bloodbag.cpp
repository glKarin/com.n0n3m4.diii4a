#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_bloodbag.h"


//Note this is the ITEM PICKUP bloodbag. The item pickup you find in the world.

//For the bloodbag that the player straps to their forehead and glugs into their
//body, all of that code is in player.cpp.




CLASS_DECLARATION(idMoveableItem, idBloodbag)
EVENT(EV_Touch, idBloodbag::Event_Touch)
END_CLASS

void idBloodbag::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( hasExploded ); //  bool hasExploded
}

void idBloodbag::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( hasExploded ); //  bool hasExploded
}

void idBloodbag::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	hasExploded = false;
	showItemLine = false;
}


void idBloodbag::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idMoveableItem::Killed(inflictor, attacker, damage, dir, location);

	//Blood explosion.
	BloodExplosion();
}


void idBloodbag::BloodExplosion()
{
	if (hasExploded)
		return;

	hasExploded = true;

	//Do this before removing objects from world.	
	idEntityFx::StartFx("fx/bloodbag_item_explosion", GetPhysics()->GetOrigin(), mat3_identity);
}

bool idBloodbag::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer())
		return false;

	gameLocal.GetLocalPlayer()->AddBloodMushroom(3, GetPhysics()->GetOrigin());
	idEntityFx::StartFx("fx/pickupitem", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

	Hide();
	physicsObj.SetContents(0);
	PostEventMS(&EV_Remove, 0);
	return true;
}