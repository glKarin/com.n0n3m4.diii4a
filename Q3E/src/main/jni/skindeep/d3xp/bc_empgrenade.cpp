#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"


#include "bc_electricalbox.h"
#include "bc_empgrenade.h"

/*

The hack grenade will look for entities that have a 'hackable' bool keyvalue.

*/

const int HG_UNDEPLOYDELAYTIME_NOHACK = 300;	//delay before it detaches itself from being bound.

const int HG_HACKRADIUS = 64; //if I'm not directly attached to entity, do a radius search XX units away.


CLASS_DECLARATION(idMoveableItem, idEMPGrenade)
	EVENT(EV_Touch, idEMPGrenade::Event_Touch)
END_CLASS

idEMPGrenade::idEMPGrenade(void)
{
}

idEMPGrenade::~idEMPGrenade(void)
{
}

void idEMPGrenade::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteTrace( collisionTr ); //  trace_t collisionTr
}

void idEMPGrenade::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadTrace( collisionTr ); //  trace_t collisionTr
}

void idEMPGrenade::Spawn(void)
{
	//non solid, but can take damage.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	state = HG_IDLE;
	stateTimer = 0;

	BecomeActive(TH_THINK);
}

//void idEMPGrenade::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
//{
//	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
//}

//void idEMPGrenade::Event_Touch(idEntity *other, trace_t *trace)
//{
//}

void idEMPGrenade::Think(void)
{
	if (state == HG_DEPLOYED)
	{
		//attached to something. Delay before it detaches.
		if (gameLocal.time >= stateTimer)
		{
			if (DoEMPBlast())
			{
				//emp blast successful.
				idAngles collisionParticleAngle = collisionTr.c.normal.ToAngles();
				collisionParticleAngle.pitch += 90;
				idEntityFx::StartFx(spawnArgs.GetString("fx_blast"), collisionTr.endpos, collisionParticleAngle.ToMat3(), NULL, false);




				//remove from world.
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
bool idEMPGrenade::Collide(const trace_t &collision, const idVec3 &velocity)
{
	collisionTr = collision;

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

		//attached. Have a delay before stuff happens.
		state = HG_DEPLOYED;
		stateTimer = gameLocal.time + HG_UNDEPLOYDELAYTIME_NOHACK;

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

void idEMPGrenade::JustThrown()
{
	//I've just been thrown by the player.
	state = HG_AIRBORNE;
	isFrobbable = false;
}

//void idEMPGrenade::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
//{
//}

//bool idEMPGrenade::DoFrob(int index, idEntity * frobber)
//{
//	return true;
//}

bool idEMPGrenade::DoEMPBlast()
{
	idLocationEntity *grenadeLocation = gameLocal.LocationForEntity(this);
	if (grenadeLocation == NULL)
		return false;

	bool appliedDamage = false;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idElectricalBox::Type))
		{
			idLocationEntity *boxLocation = gameLocal.LocationForEntity(ent);
			if (boxLocation->entityNumber == grenadeLocation->entityNumber)
			{
				//TODO: check if electricalbox is already fried.

				//Electrical box and player are in the same room. Kill the electrical box to kill the power in the room.
				static_cast<idElectricalBox *>(ent)->Damage(this, this, vec3_zero, spawnArgs.GetString("def_blastdamage"), 1.0f, 0);
				appliedDamage = true;
			}
		}
		else if (ent->spawnArgs.GetBool("emp_target"))
		{
			idLocationEntity *entLocation = gameLocal.LocationForEntity(ent);
			if (entLocation && entLocation->entityNumber == grenadeLocation->entityNumber)
			{
				ent->Damage(this, this, vec3_zero, spawnArgs.GetString("def_blastdamage"), 1000.0f, 0);
				appliedDamage = true;
			}
		}
	}

	return appliedDamage;
}
