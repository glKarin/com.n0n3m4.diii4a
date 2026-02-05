#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "bc_fireball.h"


const int ANIMTIME = 1300;
const int RADIUS = 48;

const int DELAYMAX = 250;

CLASS_DECLARATION(idAnimated, idFireball)
END_CLASS

void idFireball::Spawn(void)
{
	GetPhysics()->SetContents(0);
	fl.takedamage = false;
	timer = gameLocal.time + 10 + gameLocal.random.RandomInt(DELAYMAX);
	state = INITIALDELAY;


	//Do a clearance check, cuz it looks better when it doesn't intersect with the ground.
	trace_t floorTr;
	gameLocal.clip.TracePoint(floorTr, this->GetPhysics()->GetOrigin(), this->GetPhysics()->GetOrigin() + idVec3(0, 0, -RADIUS), MASK_SOLID, NULL);

	if (floorTr.fraction < 1)
	{
		//Ok, we're intersecting with the floor.
		//Do an upward trace to see if there's clearance above me.
		trace_t upwardTr;
		idVec3 candidatePos = floorTr.endpos + idVec3(0, 0, RADIUS+1);
		gameLocal.clip.TracePoint(upwardTr, candidatePos, candidatePos + idVec3(0,0,RADIUS), MASK_SOLID, NULL);
		if (upwardTr.fraction >= 1.0f)
		{
			//Area is clear. Move it up.
			this->GetPhysics()->SetOrigin(candidatePos);
		}
	}

	gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("def_interestpoint"));

	StartSound("snd_ignite", SND_CHANNEL_ANY, 0, false, NULL);
	idEntityFx::StartFx(spawnArgs.GetString("fx_ignite"), &this->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	BecomeActive(TH_THINK);
}



void idFireball::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( timer ); //  int timer
}

void idFireball::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( timer ); //  int timer
}

void idFireball::Think(void)
{
	idAnimated::Think();

	if (state == INITIALDELAY)
	{
		//We have a short random delay timer at the start so that if multiple fireballs get ignited, they don't all look perfectly synchronized.
		if (gameLocal.time > timer)
		{
			timer = gameLocal.time + ANIMTIME;
			state = ACTIVE;

			//Do the damage blast.
			gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, spawnArgs.GetString("def_damage"));
		}
	}
	else if (state == ACTIVE)
	{
		float lerp = (timer - gameLocal.time) / (float)(ANIMTIME);
		lerp = idMath::ClampFloat(0.0f, 1.0f, lerp);
		
		renderEntity.shaderParms[7] = lerp;
		UpdateVisuals();		
		
		if (gameLocal.time >= timer)
		{
			state = DEAD;
			this->Hide();
			this->PostEventMS(&EV_Remove, 0);
		}
	}		
}
