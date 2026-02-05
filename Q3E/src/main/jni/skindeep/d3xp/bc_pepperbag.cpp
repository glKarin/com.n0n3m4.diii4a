#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

//#include "bc_trigger_deodorant.h"
//#include "bc_trigger_sneeze.h"
#include "bc_pepperbag.h"

const int PEPPERSPEW_FALLSPEED_THRESHOLD = 200; //speed at which pepperbag must hit surface before spewing pepper.

const int PEPPERSPEW_DELAYINTERVAL = 300;


//Note: this is used for the pepperbag AND the deodorant.

CLASS_DECLARATION(idMoveableItem, idPepperBag)

END_CLASS


void idPepperBag::Spawn(void)
{
	//idMoveableItem::Spawn();

	nextFallingDamageTime = 0;

	spewType = spawnArgs.GetInt("spewtype", "0");
	if (spewType == SPEWTYPE_PEPPER || spewType == SPEWTYPE_DEODORANT)
	{
		//valid. do nothing.
	}
	else
	{
		gameLocal.Error("invalid spew type (%d) on '%s'\n", spewType, this->GetName());
	}
}

void idPepperBag::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( nextFallingDamageTime ); // int nextFallingDamageTime
	savefile->WriteInt( spewType ); // int spewType
}

void idPepperBag::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( nextFallingDamageTime ); // int nextFallingDamageTime
	savefile->ReadInt( spewType ); // int spewType
}

bool idPepperBag::Collide(const trace_t &collision, const idVec3 &velocity)
{
	float v;

	idMoveableItem::Collide(collision, velocity);	

	v = -(velocity * collision.c.normal);

	bool entityTakesDamage = gameLocal.entities[collision.c.entityNum]->fl.takedamage;

	float speedHitThreshold = entityTakesDamage ? 10 : PEPPERSPEW_FALLSPEED_THRESHOLD;

	if (v >= speedHitThreshold && gameLocal.time > nextFallingDamageTime && gameLocal.time > dropTimer + 1500 && gameLocal.time > 1500)
	{
		nextFallingDamageTime = gameLocal.time + PEPPERSPEW_DELAYINTERVAL;
		//Damage(NULL, NULL, vec3_zero, "damage_generic", 1, INVALID_JOINT); //When collision happens, make it spew out its contents.
		gameLocal.DoSpewBurst(this, spewType);
	}	

	return false;
}




void idPepperBag::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);	
	
	gameLocal.DoSpewBurst(this, spewType);
	
	if (health <= 0)
	{		
		//DEATH SPEW.
		idVec3 fxPos = GetPhysics()->GetAbsBounds().GetCenter(); //Center the explosion at bounding box middle, not origin.
		idMat3 fxMat = idAngles(0, gameLocal.random.RandomInt(360), 0).ToMat3();
		idEntityFx::StartFx(spawnArgs.GetString("fx_deathspew", "fx/pepperdeath"), &fxPos, &fxMat, NULL, false);
	}
}



bool idPepperBag::DoFrob(int index, idEntity * frobber)
{
	if (index == CARRYFROB_INDEX && frobber == gameLocal.GetLocalPlayer())
	{
		//special frob when carrying the item.
		gameLocal.DoSpewBurst(this, spewType);
		return true;
	}
	

	return idMoveableItem::DoFrob(index, frobber);	
}
