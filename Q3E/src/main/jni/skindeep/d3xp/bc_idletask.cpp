#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "ai/AI.h"
#include "bc_gunner.h"
#include "bc_idletask.h"

CLASS_DECLARATION(idEntity, idIdleTask)
END_CLASS



void idIdleTask::Spawn(void)
{
	idleAnim = spawnArgs.GetString("idleanim");

	if (idleAnim[0] == '\0')
	{
		gameLocal.Error("idleTask '%s' is missing an idleAnim.", this->GetName());
	}

	GetPhysics()->SetContents(0);
	GetPhysics()->SetClipMask(0);
	//BecomeActive(TH_THINK);
}

idIdleTask::idIdleTask(void)
{
	idletaskNode.SetOwner(this);
	idletaskNode.AddToEnd(gameLocal.idletaskEntities);
}

idIdleTask::~idIdleTask(void)
{
	idletaskNode.Remove();
}

void idIdleTask::Save(idSaveGame *savefile) const
{
	assignedActor.Save( savefile ); // idEntityPtr<idAI> assignedActor
	savefile->WriteObject( assignedOwner ); // idEntityPtr<idEntity> assignedOwner
	savefile->WriteString( idleAnim ); // idStr idleAnim
}

void idIdleTask::Restore(idRestoreGame *savefile)
{
	assignedActor.Restore( savefile ); // idEntityPtr<idAI> assignedActor
	savefile->ReadObject( assignedOwner ); // idEntityPtr<idEntity> assignedOwner
	savefile->ReadString( idleAnim ); // idStr idleAnim
}

//Assign an actor to this task. TRUE = successful move order. FALSE = failed to do move order.
bool idIdleTask::SetActor(idAI *actor)
{
	//Order the actor to move to this spot.	
	if (actor->MoveToPosition(this->GetPhysics()->GetOrigin()))
	{
		//Successful move order. Assign this entity to this actor.
		assignedActor = actor;
		return true;
	}

	return false;
}

bool idIdleTask::IsClaimed()
{
	if (assignedActor.IsValid())
		return true;

	if (assignedActor.GetEntity())
		return true;

	return false;
}
