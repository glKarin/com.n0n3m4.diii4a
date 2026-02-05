#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_bossspawnpoint.h"

CLASS_DECLARATION(idEntity, idBossSpawnpoint)
END_CLASS

void idBossSpawnpoint::Save(idSaveGame *savefile) const
{
	// savegame no data
}

void idBossSpawnpoint::Restore(idRestoreGame *savefile)
{
	// savegame no data
}


void idBossSpawnpoint::Spawn(void)
{
	
}





