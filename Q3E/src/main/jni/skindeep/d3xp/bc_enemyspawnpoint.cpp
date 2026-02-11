#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_enemyspawnpoint.h"

CLASS_DECLARATION(idEntity, idEnemySpawnpoint)
END_CLASS

void idEnemySpawnpoint::Save(idSaveGame *savefile) const
{
	// savefile no data
}

void idEnemySpawnpoint::Restore(idRestoreGame *savefile)
{
	// savefile no data
}


void idEnemySpawnpoint::Spawn(void)
{
	
}





