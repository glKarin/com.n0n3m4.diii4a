#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"

#include "bc_repairpatrol.h"

CLASS_DECLARATION(idEntity, idRepairPatrolNode)
END_CLASS

idRepairPatrolNode::idRepairPatrolNode(void)
{
	gameLocal.repairpatrolEntities.Append(this);
}

idRepairPatrolNode::~idRepairPatrolNode(void)
{
	
}

void idRepairPatrolNode::Save(idSaveGame *savefile) const
{
	// savefile no data
}

void idRepairPatrolNode::Restore(idRestoreGame *savefile)
{
	// savefile no data
}

void idRepairPatrolNode::Spawn(void)
{
}
