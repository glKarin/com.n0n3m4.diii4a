#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_seekergrenade.h"


const int SG_DEPLOYDELAYTIME = 400;	//time delay before first ball deployment.
const int SG_DEPLOYINTERVAL = 500;	//time delay between ball deployments.
const int SG_DONEDELAYTIME = 700;	//delay before it removes itself from world. We add a delay because it looks weird if it immediately disappears during the last ball deployment.

const float SG_BALLSPAWN_OFFSET = 4.0f;

CLASS_DECLARATION(idMoveableItem, idSeekerGrenade)
EVENT(EV_Touch, idSeekerGrenade::Event_Touch)
END_CLASS

idSeekerGrenade::idSeekerGrenade(void)
{
}

idSeekerGrenade::~idSeekerGrenade(void)
{
}

void idSeekerGrenade::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( ballCount ); // int ballCount

	savefile->WriteVec3( wallNormal ); // idVec3 wallNormal
}

void idSeekerGrenade::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( ballCount ); // int ballCount

	savefile->ReadVec3( wallNormal ); // idVec3 wallNormal
}

void idSeekerGrenade::Spawn(void)
{
	//non solid, but can take damage.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	isFrobbable = false;
	state = SG_AIRBORNE;
	stateTimer = 0;
	ballCount = spawnArgs.GetInt("ballCount", "5");
	wallNormal = vec3_zero;

	this->team = spawnArgs.GetInt("team");

	fl.takedamage = false;

	BecomeActive(TH_THINK);
}

//void idSeekerGrenade::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
//{
//	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
//}

//void idSeekerGrenade::Event_Touch(idEntity *other, trace_t *trace)
//{
//}

void idSeekerGrenade::Think(void)
{
	if (state == SG_DEPLOYDELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			state = SG_DEPLOYING;
			stateTimer = gameLocal.time + SG_DEPLOYINTERVAL;
		}
	}
	else if (state == SG_DEPLOYING)
	{
		if (gameLocal.time >= stateTimer)
		{
			//Deploy a ball.
			if (DeployBall())
			{
				StartSound("snd_deploy", SND_CHANNEL_ANY);

				idAngles collisionAngle = wallNormal.ToAngles();
				collisionAngle.pitch -= 90;
				gameLocal.DoParticle("smoke_sparks_01.prt", GetPhysics()->GetOrigin(), collisionAngle.ToForward());
			}
			ballCount--;

			if (ballCount <= 0)
			{
				state = SG_DONEDELAY;
				stateTimer = gameLocal.time + SG_DONEDELAYTIME;
				return;
			}

			stateTimer = gameLocal.time + SG_DEPLOYINTERVAL;
		}
	}
	else if (state == SG_DONEDELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			state = SG_DONE;
			idEntityFx::StartFx(spawnArgs.GetString("fx_death", "fx/explosion_item"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);			
			PostEventMS(&EV_Remove, 0);			
		}
	}

	idMoveableItem::Think();
}


bool idSeekerGrenade::DeployBall()
{	
	idVec3 spawnPosition = GetPhysics()->GetOrigin() + wallNormal * SG_BALLSPAWN_OFFSET;
	idEntity *ballEnt;
	idDict args;
	args.Set("classname", "projectile_seekerball");
	args.SetVector("origin", spawnPosition);
	args.SetVector("launchdir", wallNormal);
	args.SetInt("team", this->team);
	gameLocal.SpawnEntityDef(args, &ballEnt);
	if (ballEnt != NULL)
	{
		ballEnt->GetPhysics()->GetClipModel()->SetOwner(this); //to prevent ball from interacting with the grenade
		return true;
	}

	return false;
}



bool idSeekerGrenade::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (state == SG_AIRBORNE)
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
			//Don't attach to actors.
			if (GetPhysics()->GetClipModel()->GetOwner() != NULL)
			{
				if (body != GetPhysics()->GetClipModel()->GetOwner()) //If I am hitting someone who is NOT my owner, then do collision.
				{
					return idMoveableItem::Collide(collision, velocity);
				}
			}
		}

		if (gameLocal.entities[collision.c.entityNum]->IsType(idWorldspawn::Type))
		{
			//attach to worldspawn. Turn off physics.
			isBound = true;
			GetPhysics()->PutToRest(); //Turn off physics.			
		}
		else
		{
			//Attached to an entity.
			idEntity *entityToBindTo = gameLocal.entities[collision.c.entityNum];

			if (GetPhysics()->GetClipModel()->GetOwner() != NULL) //Only do bind if I'm attaching to an entity that is NOT my owner.
			{
				if (body != GetPhysics()->GetClipModel()->GetOwner())
				{
					isBound = true;
					this->Bind(entityToBindTo, true);
				}
			}
		}

		if ( !isBound )
		{
			//If it hasn't found a place to bind to, then don't collide yet.
			return false;
		}

		state = SG_DEPLOYDELAY;
		stateTimer = gameLocal.time + SG_DEPLOYDELAYTIME;
		wallNormal = collision.c.normal;

		//particle fx
		idAngles collisionParticleAngle = collision.c.normal.ToAngles();
		collisionParticleAngle.pitch += 90;		
		idEntityFx::StartFx(spawnArgs.GetString("fx_collide"), GetPhysics()->GetOrigin(), collisionParticleAngle.ToMat3(), this, true);
		StartSound("snd_impact", SND_CHANNEL_BODY2); //sound
		StartSound("snd_warnbeep", SND_CHANNEL_BODY);
		gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), -collision.c.normal, 8.0f, true, spawnArgs.GetFloat("decal_size"), "textures/decals/spearcrack"); //decal fx
						
		return idMoveableItem::Collide(collision, velocity);
	}

	//deployed, landed, or inactive.
	return false;	
}

//void idSeekerGrenade::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
//{
//}