#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "bc_trigger_gascloud.h"
#include "WorldSpawn.h"
#include "bc_algaeball.h"


#define TRACELINE_DISTANCE_THRESHOLD .3f //if traceline is below this, then skip its gas cloud
#define PROXIMITY_CHECKINTERVAL 300
#define TRIPTIMER  1000

CLASS_DECLARATION(idMoveableItem, idAlgaeball)
EVENT(EV_Touch, idAlgaeball::Event_Touch)
END_CLASS

idAlgaeball::idAlgaeball(void)
{
}

idAlgaeball::~idAlgaeball(void)
{
}

void idAlgaeball::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteObject(displayModel); //  idAnimated* displayModel

	savefile->WriteVec3( wallNormal ); //  idVec3 wallNormal

	savefile->WriteInt( spawnTime ); //  int spawnTime
}

void idAlgaeball::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadObject(CastClassPtrRef(displayModel)); //  idAnimated* displayModel

	savefile->ReadVec3( wallNormal ); //  idVec3 wallNormal

	savefile->ReadInt( spawnTime ); //  int spawnTime
}

void idAlgaeball::Spawn(void)
{
	//non solid, but can take damage.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	state = SG_AIRBORNE;
	stateTimer = 0;
	wallNormal = vec3_zero;

	this->team = spawnArgs.GetInt("team");


	spawnTime = gameLocal.time;

	idDict args;
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.Set("model", spawnArgs.GetString("model_display"));
	args.Set("anim", spawnArgs.GetString("idleanim"));
	args.SetInt("cycle", -1);
	args.SetInt("notriggeronanim", 1);
	args.SetFloat("wait", .1f);
	displayModel = (idAnimated*)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	if (displayModel)
	{
		displayModel->Bind(this, true);
	}

	BecomeActive(TH_THINK);
}

//void idAlgaeball::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
//{
//	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
//}

//void idAlgaeball::Event_Touch(idEntity *other, trace_t *trace)
//{
//}

void idAlgaeball::Think(void)
{
	if (state == SG_DEPLOYED)
	{
		//do proximity checks.
		if (gameLocal.time > stateTimer)
		{
			stateTimer = gameLocal.time + PROXIMITY_CHECKINTERVAL;

			//Am I near anyone?
			for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
			{
				if (!entity || !entity->IsActive() || entity->health <= 0 || entity->IsHidden() || entity->team == TEAM_NEUTRAL)
					continue;

				
				int triggerRadius = (entity->entityNumber == gameLocal.GetLocalPlayer()->entityNumber) ? spawnArgs.GetInt("trigger_radius_player") : spawnArgs.GetInt("trigger_radius_ai");

				float distance = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).Length();

				if (entity->IsType(idActor::Type))
				{
					float distToEye = (static_cast<idActor *>(entity)->GetEyePosition() - this->GetPhysics()->GetOrigin()).Length();
					if (distToEye < distance)
						distance = distToEye;
				}


				if (distance <= triggerRadius)
				{
					//kaboom!
					StartSound("snd_tripped", SND_CHANNEL_ANY);
					gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin(), vec3_zero);
					state = SG_TRIPPED;
					stateTimer = gameLocal.time + TRIPTIMER;
				}
			}
		}
	}
	else if (state == SG_TRIPPED)
	{
		if (gameLocal.time > stateTimer)
		{
			state = SG_DONE;
			Damage(this, this, vec3_zero, "damage_1000", 1.0f, 0);
		}
	}

	idMoveableItem::Think();
}


bool idAlgaeball::Collide(const trace_t &collision, const idVec3 &velocity)
{
	//Don't do checks if being held by player
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			return false;
		}
	}

	if (state == SG_AIRBORNE)
	{
		//We only do collision when it's in the airborne state.

		if (spawnTime + 1000 > gameLocal.time) //if it just spawned, don't do collision logic stuff
			return idMoveableItem::Collide(collision, velocity);

		bool isBound = false;

		idEntity *body = NULL;
		if (gameLocal.entities[collision.c.entityNum]->IsType(idAFAttachment::Type))
		{
			//find parent , since we hit the attachment of an actor.
			body = gameLocal.entities[collision.c.entityNum]->GetBindMaster();
		}
		else
		{
			body = gameLocal.entities[collision.c.entityNum];
		}

		if (body->IsType(idWorldspawn::Type))
		{
			//attach to worldspawn. Turn off physics.
			isBound = true;
			GetPhysics()->PutToRest(); //Turn off physics.			
		}
		else
		{
			//Attached to an entity.
			isBound = true;
			this->Bind(body, true);
		}

		if ( !isBound )
		{
			//If it hasn't found a place to bind to, then don't collide yet.
			return false;
		}

		state = SG_DEPLOYED;
		stateTimer = 0;
		wallNormal = collision.c.normal;

		//particle fx
		idAngles collisionParticleAngle = collision.c.normal.ToAngles();
		collisionParticleAngle.pitch += 90;		
		idEntityFx::StartFx(spawnArgs.GetString("fx_collide"), GetPhysics()->GetOrigin(), collisionParticleAngle.ToMat3(), this, true);
		gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), -collision.c.normal, 8.0f, true, spawnArgs.GetFloat("decal_size"), spawnArgs.GetString("mtr_splat")); //decal fx
						
		return idMoveableItem::Collide(collision, velocity);
	}	

	//deployed, landed, or inactive.
	return false;	
}

bool idAlgaeball::DoFrob(int index, idEntity* frobber)
{
	state = SG_AIRBORNE;
	return idMoveableItem::DoFrob(index, frobber);
}

void idAlgaeball::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	DoGasBurst();
}

void idAlgaeball::DoGasBurst()
{
	gameLocal.AddEventLog("#str_def_gameplay_algaeexplode", GetPhysics()->GetOrigin());

	StartSound("snd_spew", SND_CHANNEL_BODY);

	idStr interestPointDef;
	if (spawnArgs.GetString("def_interestpoint", "", interestPointDef))
	{
		gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), interestPointDef);
	}

	#define SPHEREDENSITY 5
	idVec3* pointsOnSphere;
	pointsOnSphere = gameLocal.GetPointsOnSphere(SPHEREDENSITY);
	int spewRadius = spawnArgs.GetInt("spewRadius", "32");
	int cloudRadius = spawnArgs.GetInt("cloudRadius", "32");
	int cloudsSpawned = 0;
	for (int i = 0; i < SPHEREDENSITY; i++)
	{
		idVec3 wireDir;
		wireDir = pointsOnSphere[i];
		wireDir.Normalize();

		idVec3 endPos = GetPhysics()->GetOrigin() + (wireDir * spewRadius);

		trace_t tr;
		gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), endPos, MASK_SOLID, NULL);
		
		if (tr.fraction <= TRACELINE_DISTANCE_THRESHOLD) //if the traceline is very short, then just don't do it
			continue;		

		
		SpawnSingleCloud(tr.endpos, cloudRadius);
		cloudsSpawned++;
	}

	if (cloudsSpawned <= 0)
	{
		//Somehow failed to spew any clouds at all. Fallback: just spawn a single cloud at my origin.
		SpawnSingleCloud(GetPhysics()->GetOrigin(), cloudRadius);
	}
}

void idAlgaeball::SpawnSingleCloud(idVec3 _pos, float _cloudRadius)
{
	idDict args;
	args.Clear();
	args.SetVector("origin", _pos);
	args.SetVector("mins", idVec3(-_cloudRadius, -_cloudRadius, -_cloudRadius));
	args.SetVector("maxs", idVec3(_cloudRadius, _cloudRadius, _cloudRadius));

	idTrigger_gascloud* spewTrigger;
	args.Set("callOnStun", spawnArgs.GetString("callOnStun", "")); // SW: adding support for algae balls to call scripts when they catch someone
	args.Set("spewParticle", spawnArgs.GetString("spewParticle", "gascloud01.prt"));
	args.SetInt("spewLifetime", spawnArgs.GetInt("spewLifetime", "9000"));
	spewTrigger = (idTrigger_gascloud*)gameLocal.SpawnEntityType(idTrigger_gascloud::Type, &args);
}

void idAlgaeball::Hide(void)
{
	displayModel->Hide();
	idMoveableItem::Hide();	
}

void idAlgaeball::Show(void)
{
	displayModel->Show();
	idMoveableItem::Show();
}