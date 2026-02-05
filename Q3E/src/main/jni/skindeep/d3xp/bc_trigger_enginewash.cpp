#include "Trigger.h"
#include "Player.h"

#include "bc_meta.h"
#include "bc_trigger_enginewash.h"

CLASS_DECLARATION(idTrigger_Multi, idTrigger_enginewash)
EVENT(EV_Touch, idTrigger_enginewash::Event_Touch)
END_CLASS

#define PUSHINTERVAL 300
#define DAMAGEINTERVAL 20
#define PUSHFORCE 80

#define CLOSE_THRESHOLD 48


idTrigger_enginewash::idTrigger_enginewash()
{
}

idTrigger_enginewash::~idTrigger_enginewash(void)
{
}

void idTrigger_enginewash::Spawn()
{
	idTrigger_Multi::Spawn();
	timer = 0;
	baseAngle = spawnArgs.GetFloat("baseangle");

	if (baseAngle == 0)
		baseAngle = 1;

	state = EW_NONE;
	BecomeActive(TH_THINK);
}


void idTrigger_enginewash::Save(idSaveGame* savefile) const
{
	savefile->WriteFloat( baseAngle ); // float baseAngle
	savefile->WriteInt( timer ); // int timer
	savefile->WriteInt( state ); // int state
}
void idTrigger_enginewash::Restore(idRestoreGame* savefile)
{
	savefile->ReadFloat( baseAngle ); // float baseAngle
	savefile->ReadInt( timer ); // int timer
	savefile->ReadInt( state ); // int state
}

void idTrigger_enginewash::Think(void)
{
	if (state == EW_NONE)
	{
		if (gameLocal.time > timer)
		{
			state = EW_PUSHING;
			timer = gameLocal.time + DAMAGEINTERVAL;
		}
	}
	else if (state == EW_PUSHING)
	{
		if (gameLocal.time > timer)
		{
			state = EW_NONE;
			timer = gameLocal.time + PUSHINTERVAL;
		}
	}
}

void idTrigger_enginewash::Event_Touch(idEntity* other, trace_t* trace)
{
	if (state != EW_PUSHING)
		return;

	//See if it's close to the "booster" part. If so, we apply more force and more damage.
	//Assumptions: the ship is facing northward, the booster is pushing southward. So we just check Y position values.
	float boosterPosY = GetPhysics()->GetAbsBounds()[1].y;
	//idVec3 boosterPos;
	//boosterPos = GetPhysics()->GetAbsBounds().GetCenter();
	//boosterPos.y = GetPhysics()->GetAbsBounds()[1].y;

	//We now have the position of where the booster flame originates from.
	//Get delta between booster Y position & entity Y position.
	float delta = idMath::Fabs(other->GetPhysics()->GetOrigin().y - boosterPosY);

	//Is close to the booster. So apply additional force/damage.
	bool isCloseToBooster = delta < CLOSE_THRESHOLD;
	float amountToPush = isCloseToBooster ? PUSHFORCE * 4 : PUSHFORCE;
	float damageMultiplier = isCloseToBooster ? 10.0f : 1.0f;	

	//Apply damage & force.
	idAngles pushDir = idAngles(0, baseAngle, 0);
	other->ApplyImpulse(other, 0, other->GetPhysics()->GetAbsBounds().GetCenter(), pushDir.ToForward() * amountToPush * other->GetPhysics()->GetMass());
	other->Damage(this, this, vec3_zero, spawnArgs.GetString("def_damage"), damageMultiplier, 0);
}
