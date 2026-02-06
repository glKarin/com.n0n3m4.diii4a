#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"



#include "bc_hackgrenade.h"

/*

The hack grenade will look for entities that have a 'hackable' bool keyvalue.

*/

const int HG_UNDEPLOYDELAYTIME = 2500;	//delay before it detaches itself from being bound.
const int HG_UNDEPLOYDELAYTIME_NOHACK = 300;	//delay before it detaches itself from being bound.

const int HG_HACKRADIUS = 64; //if I'm not directly attached to entity, do a radius search XX units away.


CLASS_DECLARATION(idMoveableItem, idHackGrenade)
	EVENT(EV_Touch, idHackGrenade::Event_Touch)
END_CLASS

idHackGrenade::idHackGrenade(void)
{
}

idHackGrenade::~idHackGrenade(void)
{
}

void idHackGrenade::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteBool( hackSuccessful ); // bool hackSuccessful
}

void idHackGrenade::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadBool( hackSuccessful ); // bool hackSuccessful
}

void idHackGrenade::Spawn(void)
{
	//non solid, but can take damage.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	state = HG_IDLE;
	stateTimer = 0;
	hackSuccessful = false;

	BecomeActive(TH_THINK);
}

//void idHackGrenade::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
//{
//	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
//}

//void idHackGrenade::Event_Touch(idEntity *other, trace_t *trace)
//{
//}

void idHackGrenade::Think(void)
{
	if (state == HG_DEPLOYED)
	{
		//attached to something. Delay before it detaches.
		if (gameLocal.time >= stateTimer)
		{
			if (hackSuccessful)
			{
				//if hack was successful, just make it disappear for now.
				PostEventMS(&EV_Remove, 0);
			}
			else
			{
				//just unpluck from wall and fall to floor.
				state = HG_DEPLETED;
				Unbind(); //detach from whatever I'm bound to.
				GetPhysics()->Activate();
				isFrobbable = true;
			}
		}
	}

	idMoveableItem::Think();
}

//Detect when it touches something.
bool idHackGrenade::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (state == HG_AIRBORNE)
	{
		//We only do collision when it's in the airborne state.

		bool isBound = false;

		idEntity *body;
		if (gameLocal.entities[collision.c.entityNum]->IsType(idAFAttachment::Type))
		{
			//find parent , since we hit the attachment of an actor.
			body = gameLocal.entities[collision.c.entityNum]->GetBindMaster();
		}
		else
		{
			body = gameLocal.entities[collision.c.entityNum];
		}

		if (body->IsType(idAFEntity_Base::Type))
		{
			//attach to actor.
			isBound = true;
			this->Bind(body, true);		
		}

		if (gameLocal.entities[collision.c.entityNum]->IsType(idWorldspawn::Type))
		{
			//attach to worldspawn. Turn off physics.
			isBound = true;
			GetPhysics()->PutToRest(); //Turn off physics.			
		}
		else if (!isBound)
		{
			//Attached to an entity.
			idEntity *entityToBindTo = gameLocal.entities[collision.c.entityNum];			
			isBound = true;
			this->Bind(entityToBindTo, true);			
		}

		if ( !isBound )
		{
			//If it hasn't found a place to bind to, then don't collide yet.
			return false;
		}

		if (StartHack(collision.c.entityNum))
		{
			isFrobbable = false;
			hackSuccessful = true;
			StartSound("snd_hack", SND_CHANNEL_BODY);
		}

		//attached. Have a delay before we detach.
		state = HG_DEPLOYED;
		stateTimer = gameLocal.time + (hackSuccessful ? HG_UNDEPLOYDELAYTIME : HG_UNDEPLOYDELAYTIME_NOHACK); //if successful hack, then stay stuck to the surface for a longer amount of time.

		//particle fx
		idAngles collisionParticleAngle = collision.c.normal.ToAngles();
		collisionParticleAngle.pitch += 90;		
		idEntityFx::StartFx(spawnArgs.GetString("fx_collide"), GetPhysics()->GetOrigin(), collisionParticleAngle.ToMat3(), this, true);
		StartSound("snd_impact", SND_CHANNEL_BODY2); //sound		
		gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), -collision.c.normal, 8.0f, true, spawnArgs.GetFloat("decal_size"), "textures/decals/scorch_faded"); //decal fx
						
		return idMoveableItem::Collide(collision, velocity);
	}

	//deployed, landed, or inactive.
	return false;	
}

bool idHackGrenade::StartHack(int entityNum)
{
	//First, we see if the thing we are directly attached to is hackable.
	if (AttemptHackDirect(entityNum))
		return true;

	//Ok, try again. See if there's any hackable things nearby.
	if (AttemptHackRadius())
		return true;

	//Fail. Don't hack anything.
	return false;
}

bool idHackGrenade::AttemptHackDirect(int entityNum)
{
	if (entityNum <= MAX_GENTITIES - 2 && entityNum >= 0)
	{
		idEntity *ent = gameLocal.entities[entityNum];

		if (!ent)
			return false;

		if (ent->team == TEAM_FRIENDLY)
			return false; //is a friendly. Don't do anything.

		return HackEntity(ent);
	}

	return false;
}

bool idHackGrenade::AttemptHackRadius()
{
	//So, my landing position was kinda sloppy and I'm not directly touching something that's hackable.
	//Try to see if there's anything nearby that we can hack.

	//First, generate a list of all things nearby.
	idStaticList<spawnSpot_t, MAX_GENTITIES> candidates;

	int entityCount;
	idEntity *entityList[MAX_GENTITIES];
	int i;

	entityCount = gameLocal.EntitiesWithinRadius(GetPhysics()->GetOrigin(), HG_HACKRADIUS, entityList, MAX_GENTITIES);

	if (entityCount <= 0)
		return false;

	for (i = 0; i < entityCount; i++)
	{
		idEntity *ent = entityList[i];

		if (!ent)
			continue;

		if (ent->IsHidden() || ent == this || ent->health <= 0 || ent->team		== TEAM_FRIENDLY)
			continue;

		//TODO: make sure it's in the same room and/or PVS

		//is a possible candidate.
		spawnSpot_t newcandidate;
		newcandidate.ent = ent;
		newcandidate.dist = (GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin()).LengthFast();
		candidates.Append(newcandidate);
	}

	if (candidates.Num() <= 0)
		return false;

	//Great, we now have a list of nearby entities.
	//We want to hack the NEAREST thing. So, start by sorting the entities by distance.
	qsort((void *)candidates.Ptr(), candidates.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Nearest);

	//Now that the list is sorted by distance, we iterate through this
	//list and try to find the first thing we can hack.
	for (int i = 0; i < candidates.Num(); i++)
	{
		if (HackEntity(candidates[i].ent))
		{
			//successful hack.
			return true;
		}
	}

	return false;
}


bool idHackGrenade::HackEntity(idEntity *ent)
{
	if (ent->spawnArgs.GetBool("hackable", "0"))
	{
		gameLocal.DoParticle(spawnArgs.GetString("model_hackparticle"), ent->GetPhysics()->GetOrigin());

		ent->DoHack();
		return true;
	}

	return false;
}


void idHackGrenade::JustThrown()
{
	//I've just been thrown by the player.

	state = HG_AIRBORNE;
}

//void idHackGrenade::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
//{
//}

//bool idHackGrenade::DoFrob(int index, idEntity * frobber)
//{
//	return true;
//}