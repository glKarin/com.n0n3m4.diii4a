#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_nudgepoint.h"

CLASS_DECLARATION(idEntity, idNudgePoint)
END_CLASS

idNudgePoint::idNudgePoint(void)
{
	spacenudgeNode.SetOwner(this);
	spacenudgeNode.AddToEnd(gameLocal.spacenudgeEntities);
}

idNudgePoint::~idNudgePoint(void)
{
	spacenudgeNode.Remove();
}

void idNudgePoint::Save(idSaveGame *savefile) const
{
	// savegame no data
}

void idNudgePoint::Restore(idRestoreGame *savefile)
{
	// savegame no data
}


void idNudgePoint::Spawn(void)
{
}

	