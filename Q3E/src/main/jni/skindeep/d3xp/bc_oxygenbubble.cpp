#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "bc_oxygenbubble.h"

const int MOVESPEED = 64;

const int BUBBLESTARTSIZE = 4;
const int BUBBLEENDSIZE = 1;
const int BUBBLE_SPAWNTIME = 300;

CLASS_DECLARATION(idMoveableItem, idOxygenbubble)
END_CLASS


idOxygenbubble::idOxygenbubble(void)
{
}

idOxygenbubble::~idOxygenbubble(void)
{

}

void idOxygenbubble::Spawn(void)
{
	physicsObj.SetFriction(.1f, .1f, .1f);
		
	GetPhysics()->SetGravity(idVec3(0, 0, 0));
	GetPhysics()->SetContents(CONTENTS_TRIGGER);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	fl.takedamage = true;
	bubbleState = OB_SPAWNING;
	bubbleTimer = gameLocal.time + BUBBLE_SPAWNTIME;

	idVec3 moveDir = spawnArgs.GetVector("movedir");
	GetPhysics()->SetLinearVelocity(moveDir * -MOVESPEED);
}

void idOxygenbubble::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( bubbleState ); // int bubbleState
	savefile->WriteInt( bubbleTimer ); // int bubbleTimer
}

void idOxygenbubble::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( bubbleState ); // int bubbleState
	savefile->ReadInt( bubbleTimer ); // int bubbleTimer
}

void idOxygenbubble::Think(void)
{
	idMoveableItem::Think();	

	if (bubbleState == OB_SPAWNING)
	{
		float lerp = (bubbleTimer - gameLocal.time) / (float)(BUBBLE_SPAWNTIME);
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = 1 - lerp;

		GetRenderEntity()->shaderParms[7] = idMath::Lerp(BUBBLESTARTSIZE, BUBBLEENDSIZE, lerp);

		if (lerp >= 1)
		{
			bubbleState = OB_IDLE;
		}
	}
	else if (bubbleState == OB_IDLE)
	{
		if (this->GetPhysics()->GetLinearVelocity().LengthFast() < THROWABLE_DRIFT_THRESHOLD)
		{
			//has stopped moving.
			Killed(this, this, 100, vec3_zero, 0);
		}
	}
}


void idOxygenbubble::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	idMoveableItem::Killed(inflictor, attacker, 1, dir, location);
}

void idOxygenbubble::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (bubbleState == OB_DEAD)
		return;

	bubbleState = OB_DEAD;

	idEntityFx::StartFx("fx/oxygenbubble_pop", GetPhysics()->GetOrigin(), mat3_identity);

	idMoveableItem::Killed(inflictor, attacker, damage, dir, location);
}
