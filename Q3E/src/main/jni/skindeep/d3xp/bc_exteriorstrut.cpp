#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "bc_exteriorstrut.h"


CLASS_DECLARATION(idAnimated, idExteriorStrut)
END_CLASS


void idExteriorStrut::Spawn(void)
{
	GetPhysics()->SetContents(0);
	fl.takedamage = false;	

	armJoint = animator.GetJointHandle("arm");
	if (armJoint == INVALID_JOINT)
	{
		gameLocal.Error("exteriorstrut '%s' failed to set up joints.", GetName());
	}

	PostEventMS(&EV_PostSpawn, 0);
}

void idExteriorStrut::Event_PostSpawn(void)
{
	if (targets.Num() <= 0)
	{
		gameLocal.Error("exteriorstrut '%s' has no target.", GetName());
		return;
	}

	idEntity *targetEnt = targets[0].GetEntity();

	if (!targetEnt)
	{
		gameLocal.Error("exteriorstrut '%s' has bad target.", GetName());
		return;
	}


	//Create a null ent and bind it to the mover entity. This null ent is what we move along with.
	idDict args;
	args.Clear();
	args.Set("classname", "target_null");
	args.SetInt("angle", this->GetPhysics()->GetAxis().ToAngles().yaw + 180);
	gameLocal.SpawnEntityDef(args, &nullEnt);

	if (!nullEnt)
	{
		gameLocal.Error("exteriorstrut '%s' failed to spawn null ent.", this->GetName());
	}

	idVec3 armPos;
	idMat3 armMat;
	this->GetJointWorldTransform(armJoint, gameLocal.time, armPos, armMat);
	nullEnt->SetOrigin(armPos);
	nullEnt->Bind(targetEnt, true);

	BecomeActive(TH_THINK);
}

void idExteriorStrut::Save(idSaveGame *savefile) const
{
	savefile->WriteJoint( armJoint ); //  saveJoint_t armJoint
	savefile->WriteObject( nullEnt ); //  idEntity * nullEnt
}

void idExteriorStrut::Restore(idRestoreGame *savefile)
{
	savefile->ReadJoint( armJoint ); //  saveJoint_t armJoint
	savefile->ReadObject( nullEnt ); //  idEntity * nullEnt
}

void idExteriorStrut::Think(void)
{
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	idAnimated::Think();

	idVec3 armPos = nullEnt->GetPhysics()->GetOrigin();
	armPos = (armPos - renderEntity.origin) * renderEntity.axis.Transpose();
	animator.SetJointPos(armJoint, JOINTMOD_WORLD_OVERRIDE, armPos);	

	//TODO: fix this. Angle isn't working.
	//uhhhhhhhhhhhhhhh this sucks
	//idAngles nullAngle = nullEnt->GetPhysics()->GetAxis().ToAngles();
	//nullAngle.yaw += 180;
	//nullAngle.pitch *= -1.0f;
	//idMat3 armAngle = nullAngle.ToMat3() * renderEntity.axis.Transpose();
	//animator.SetJointAxis(armJoint, JOINTMOD_WORLD, armAngle);
}
