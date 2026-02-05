#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"
#include "SmokeParticles.h"
#include "framework/DeclEntityDef.h"
#include "Projectile.h"


#include "ai/AI.h"
#include "bc_mech.h"
#include "bc_spearprojectile.h"

#define SPEARCHARGETIME 500

//The spearbot is turned into a spearprojectile. The spearprojectile is what the spearbot turns into when it flies toward its target.

CLASS_DECLARATION(idMoveableItem, idSpearprojectile)
END_CLASS

idSpearprojectile::idSpearprojectile(void)
{

}

idSpearprojectile::~idSpearprojectile(void)
{
	if (impaledInMonster && gameLocal.GameState() != GAMESTATE_SHUTDOWN)
	{
		//the impaled monster got destroyed somehow. Make the spearfish explode its items/gibs.
		const char *projectileDefname = spawnArgs.GetString("def_projectile");
		if (projectileDefname[0] != '\0')
		{
			const idDeclEntityDef *projectileDef;
			projectileDef = gameLocal.FindEntityDef(projectileDefname, false);
			if (projectileDef)
			{
				//const char *itemName = projectileDef->dict.GetString("spawn_on_explode");
				//const char *debrisName = projectileDef->dict.GetString("def_debris");
				//int debrisCount = projectileDef->dict.GetInt("debris_count");

				//Spawn projectile and immediately make it explode.
				idEntity *projectile = gameLocal.LaunchProjectile(gameLocal.GetLocalPlayer(), projectileDefname, GetPhysics()->GetOrigin(), vec3_zero);
				if (projectile->IsType(idProjectile::Type))
				{
					trace_t tr;
					gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin() + idVec3(0, 0, 2), GetPhysics()->GetOrigin(), MASK_SOLID, NULL);
					static_cast<idProjectile *>(projectile)->Explode(tr, NULL);
				}
			}
		}		
	}
}

void idSpearprojectile::Spawn(void)
{
	GetPhysics()->SetGravity(idVec3(0,0,0));

	//Set up the ram info.
	ramPoint = spawnArgs.GetVector("rampoint");
	ramSpeed = spawnArgs.GetInt("ramspeed");
	spearState = SPEAR_RAMMING;
	ramDir = ramPoint - GetPhysics()->GetOrigin();
	ramDir.Normalize();

	//Set angle.
	idVec3 dirToTarget = ramPoint - GetPhysics()->GetOrigin();
	dirToTarget.Normalize();
	this->SetAxis(dirToTarget.ToMat3());

	//Set up the smoke trail.
	const char *smokeName = spawnArgs.GetString("smoke_fly");
	flyParticlesFlyTime = 0;
	if (*smokeName != '\0')
	{
		flyParticles = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, smokeName));
		flyParticlesFlyTime = gameLocal.time;
	}
	
	pluckedFromBelly = false;
	enemyPtr = NULL;
	chargeTimer = 0;
    collisionNormal = vec3_zero;
	hasdoneSpawnBoundCheck = false;
	impaledInMonster = false;
	StartSound("snd_launch", SND_CHANNEL_BODY2);
	BecomeActive(TH_THINK);

	idEntity *ownerEnt = gameLocal.FindEntity(spawnArgs.GetString("owner"));
	if (ownerEnt != NULL)
	{
		physicsObj.GetClipModel()->SetOwner(ownerEnt);
	}
}

void idSpearprojectile::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( spearState ); // int spearState
	savefile->WriteVec3( ramPoint ); // idVec3 ramPoint
	savefile->WriteInt( ramSpeed ); // int ramSpeed
	savefile->WriteVec3( ramDir ); // idVec3 ramDir

	savefile->WriteParticle( flyParticles ); // const idDeclParticle * flyParticles
	savefile->WriteInt( flyParticlesFlyTime ); // int flyParticlesFlyTime

	savefile->WriteBool( hasdoneSpawnBoundCheck ); // bool hasdoneSpawnBoundCheck


	savefile->WriteVec3( collisionNormal ); // idVec3 collisionNormal

	savefile->WriteInt( chargeTimer ); // int chargeTimer

	savefile->WriteObject( enemyPtr ); // idEntityPtr<idEntity> enemyPtr

	savefile->WriteBool( pluckedFromBelly ); // bool pluckedFromBelly

	savefile->WriteBool( impaledInMonster ); // bool impaledInMonster
}

void idSpearprojectile::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( spearState ); // int spearState
	savefile->ReadVec3( ramPoint ); // idVec3 ramPoint
	savefile->ReadInt( ramSpeed ); // int ramSpeed
	savefile->ReadVec3( ramDir ); // idVec3 ramDir

	savefile->ReadParticle( flyParticles ); // const idDeclParticle * flyParticles
	savefile->ReadInt( flyParticlesFlyTime ); // int flyParticlesFlyTime

	savefile->ReadBool( hasdoneSpawnBoundCheck ); // bool hasdoneSpawnBoundCheck


	savefile->ReadVec3( collisionNormal ); // idVec3 collisionNormal

	savefile->ReadInt( chargeTimer ); // int chargeTimer

	savefile->ReadObject( enemyPtr ); // idEntityPtr<idEntity> enemyPtr

	savefile->ReadBool( pluckedFromBelly ); // bool pluckedFromBelly

	savefile->ReadBool( impaledInMonster ); // bool impaledInMonster
}

void idSpearprojectile::Think(void)
{
	if (!hasdoneSpawnBoundCheck)
	{
		hasdoneSpawnBoundCheck = true;

		//Ok, when first spawned into world, do a check to see if we're colliding with anything.
		//This is to handle cases where I spawn right next to the ground. We want to do the collision immediately.


		trace_t spawnTr;
		float forwardBoundDistance = GetPhysics()->GetBounds()[1].x;
		idVec3 forwardDir;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);

		gameLocal.clip.TracePoint(spawnTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + forwardDir * forwardBoundDistance, MASK_SOLID, this);
		
		if (spawnTr.fraction < 1)
		{
			//Immediate collision.
			Collide(spawnTr, vec3_zero);
		}
	}

	if (spearState == SPEAR_RAMMING)
	{
		idVec3 force;		
		force = (ramDir * ramSpeed) * this->GetPhysics()->GetMass();
		this->ApplyImpulse(NULL, 0, GetPhysics()->GetOrigin(), force);
		if (flyParticles != NULL && flyParticlesFlyTime && !IsHidden())
		{
			idVec3 dir = idVec3(0, 1, 0);

			if (!gameLocal.smokeParticles->EmitSmoke(flyParticles, flyParticlesFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup))
			{
				flyParticlesFlyTime = gameLocal.time;
			}
		}

		RunPhysics();
	}
	else if (spearState == SPEAR_CHARGING)
	{
		if (gameLocal.time >= chargeTimer)
		{
			Launch();
			spearState = SPEAR_DONE;
		}
	}

	Present();
}


bool idSpearprojectile::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (gameLocal.entities[collision.c.entityNum] == NULL)
		return false;

	if (gameLocal.entities[collision.c.entityNum]->IsType(idSpearprojectile::Type))
		return false;

	if (spearState == SPEAR_RAMMING)
	{
		//Impale!!!!!!
		idVec3 forwardDir;
		idAngles particleDir;
		idEntity *body;
		idVec3 pushForce;
		
		spearState = SPEAR_IMPALED;
        collisionNormal = collision.c.normal; 

		//Spawn interestpoint when I collide.
		gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_collide"));

		//Dig it in a little further.
		forwardDir = GetPhysics()->GetAxis().ToAngles().ToForward();
		GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() + forwardDir * 24);
		
		//Immediately stop all movement.
		GetPhysics()->SetLinearVelocity(vec3_zero);
		GetPhysics()->SetAngularVelocity(vec3_zero);

		//particles.
		particleDir = forwardDir.ToAngles();
		particleDir.yaw += 180;
	
		if (gameLocal.entities[collision.c.entityNum]->IsType(idAFAttachment::Type))
		{
			//find parent , since we hit the attachment of an actor.
			body = gameLocal.entities[collision.c.entityNum]->GetBindMaster();
		}
		else
		{
			body = gameLocal.entities[collision.c.entityNum];
		}

		if (body->IsType(idAFEntity_Base::Type) )
		{
			idDict splashArgs;
			idFuncEmitter *splashEnt;
			idVec3 bloodPos;

			//Ok, check if player is jockeying. If player is jockeying, and is hit with a spearfish from the FRONT, then we
			//instead make the spearfish do damage to the jockee.
			if (body->IsType(idPlayer::Type) && gameLocal.GetLocalPlayer()->IsJockeying() && gameLocal.GetLocalPlayer()->meleeTarget.IsValid())
			{
				//spearfish hit player...				
				
				idVec3 jockeeDir = gameLocal.GetLocalPlayer()->meleeTarget.GetEntity()->viewAxis.ToAngles().ToForward();
				float dotResult = DotProduct(forwardDir, jockeeDir);

				if (dotResult < JOCKEY_REAR_DAMAGE_DOTTHRESHOLD)
				{
					//spearfish attack coming from FRONT.
					//divert the spearfish damage to the jockee.
					//body = gameLocal.GetLocalPlayer()->meleeTarget.GetEntity();

					const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_damage"), false);
					if (damageDef)
					{
						int meleeNum = gameLocal.GetLocalPlayer()->meleeTarget.GetEntityNum();
						gameLocal.entities[meleeNum]->Damage(this, this, vec3_zero, spawnArgs.GetString("def_damage"), 1.0f, 0);
					}

					PostEventMS(&EV_Remove, 0);
					return true;
				}
			}

			//Attempt to bind to chest joint.
			jointHandle_t chestJoint = body->GetAnimator()->GetJointHandle("chest");			

			//Bind it to the entity. Probably being bound to a monster.
			if (chestJoint != INVALID_JOINT)
			{
				//Hit the chest. Bind to chest.
				idVec3 chestPos;
				idMat3 chestAxis;

				((idAnimatedEntity *)body)->GetJointWorldTransform(chestJoint, gameLocal.time, chestPos, chestAxis);
				GetPhysics()->SetOrigin(chestPos);
				this->BindToJoint(body, chestJoint, true);
				
				//gameRenderWorld->DebugArrow(colorGreen, chestPos + idVec3(0, 0, 64), chestPos, 2, 20000);				
			}
			else
			{
				//Bind to the closest joint.
				jointHandle_t jointID = CLIPMODEL_ID_TO_JOINT_HANDLE(collision.c.id);

				if (jointID != INVALID_JOINT)
				{
					//Bind to joint.
					this->BindToJoint(body, jointID, true);
				}
				else
				{
					//Generic bind.
					this->Bind(body, true);
				}
			}

			if (g_bloodEffects.GetBool())
			{
				bloodPos = GetPhysics()->GetOrigin() + forwardDir * 10;

				splashArgs.Set("model", "spearbot_blood.prt");
				splashArgs.Set("start_off", "1");
				splashEnt = static_cast<idFuncEmitter*>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));
				splashEnt->GetPhysics()->SetOrigin(bloodPos);
				splashEnt->GetPhysics()->SetAxis(particleDir.ToMat3());
				splashEnt->PostEventMS(&EV_Activate, 0, this);
				splashEnt->PostEventMS(&EV_Remove, 3000);
				splashEnt->Bind(body, true);
			}

			StartSound("snd_impact_flesh", SND_CHANNEL_ANY, 0, false, NULL);

			//Give a little physics push.
			pushForce = forwardDir * 1024 * body->GetPhysics()->GetMass();
			body->GetPhysics()->ApplyImpulse(0, body->GetPhysics()->GetAbsBounds().GetCenter(), pushForce);

			if (body->IsType(idPlayer::Type))
			{
				//((idPlayer *)body)->SetFallState(true);
				((idPlayer *)body)->SetSpearwound();
			}
			else
			{
				if (GetBindMaster() != NULL)
				{
					impaledInMonster = true;
				}
			}

			this->isFrobbable = false;
		}
		else
		{
			//Embed in a surface.
			//TODO: don't bind if it's worldspawn.
			this->Bind(gameLocal.entities[collision.c.entityNum], true);

			idAngles collisionParticleAngle = forwardDir.ToAngles();
			collisionParticleAngle.pitch -= 90;

			idEntityFx::StartFx(spawnArgs.GetString("fx_collide"), collision.endpos, collisionParticleAngle.ToMat3(), this, true);
			StartSound("snd_impact", SND_CHANNEL_ANY, 0, false, NULL);

			gameLocal.ProjectDecal(collision.c.point, -collision.c.normal, 8.0f, true, 64.0f, "textures/decals/spearcrack");

			this->isFrobbable = true;
		}

		//BC 2-18-2025: if spear hit mech, then do no damage and just blow up
		bool hitPlayerInMech = ((body->IsType(idPlayer::Type) || body->IsType(idMech::Type)) && gameLocal.GetLocalPlayer()->IsInMech());
		if (gameLocal.entities[collision.c.entityNum]->fl.takedamage && !hitPlayerInMech)
		{
			const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_damage"), false);
			if (damageDef)
			{
				gameLocal.entities[collision.c.entityNum]->Damage(this, this, vec3_zero, spawnArgs.GetString("def_damage"), 1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(collision.c.id));
			}
		}

		if (hitPlayerInMech)
		{
			PostEventMS(&EV_Remove, 1000);
		}
	}

	return false;
}


void idSpearprojectile::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
}

bool idSpearprojectile::DoFrob(int index, idEntity * frobber)
{
	if (spearState != SPEAR_IMPALED)
		return false;

	//special de-impalement interaction from idPlayer::StartHealState()
	if (index == DEIMPALE_FROBINDEX)
	{
		//Player is pulling spear out of belly.
		pluckedFromBelly = true;
	}


	StartSound("snd_angry", SND_CHANNEL_VOICE);
	spearState = SPEAR_CHARGING;
	chargeTimer = gameLocal.time + SPEARCHARGETIME;
	idEntityFx::StartFx("fx/smoke_sparks_03_2sec", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	return true;
}

void idSpearprojectile::Launch()
{
	idVec3 forwardDir;
	idAngles particleDir;

	forwardDir = GetPhysics()->GetAxis().ToAngles().ToForward();
	particleDir = forwardDir.ToAngles();
	particleDir.yaw += 180;

	idEntityFx::StartFx("fx/smoke_sparks_01", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	this->Hide();
	PostEventMS(&EV_Remove, 0);


	if (pluckedFromBelly)
	{
		//Frob spearbot impaled in belly.

		idVec3 trajectory = gameLocal.GetLocalPlayer()->viewAngles.ToForward();
		gameLocal.LaunchProjectile(gameLocal.GetLocalPlayer(), spawnArgs.GetString("def_projectile"), GetPhysics()->GetOrigin() + trajectory * 4, trajectory);
	}
	else
	{
		//Frob spearbot on the ground/wall/world.

		//Spawn and launch the projectile.
		idVec3 spawnPosition = FindValidSpawnPosition();
		if (spawnPosition != vec3_zero)
		{
			idVec3 trajectory = FindValidTrajectory(spawnPosition);
			idEntity *projectile = gameLocal.LaunchProjectile(NULL, spawnArgs.GetString("def_projectile"), spawnPosition, trajectory);
			if (projectile != NULL)
			{
				if (projectile->IsType(idGuidedProjectile::Type) && enemyPtr.IsValid())
				{
					if (enemyPtr.GetEntity()->health > 0 && !enemyPtr.GetEntity()->IsHidden())
					{
						static_cast<idGuidedProjectile *>(projectile)->SetEnemy(enemyPtr.GetEntity());
					}
				}
			}
		}
	}
}


idVec3 idSpearprojectile::FindValidTrajectory(idVec3 spawnPosition)
{
	float closestDistance = 9999;
	idVec3 targetPosition = vec3_zero;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0 || entity->team == this->team || entity == gameLocal.GetLocalPlayer())
			continue;

		//Check LOS.
		trace_t losTr;
		idVec3 candidatePosition = entity->GetPhysics()->GetAbsBounds().GetCenter();
		gameLocal.clip.TracePoint(losTr, spawnPosition, candidatePosition, MASK_SOLID, this);
		if (losTr.fraction >= 1)
		{
			float currentDistance = (spawnPosition - targetPosition).LengthFast();
			if (currentDistance < closestDistance)
			{
				closestDistance = currentDistance;
				targetPosition = candidatePosition;
				enemyPtr = entity;
			}
		}		
	}

	if (targetPosition == vec3_zero)
	{
		//fail.
		return collisionNormal;
	}

	idVec3 trajectory = targetPosition - spawnPosition;
	trajectory.Normalize();
	return trajectory;
}




idVec3 idSpearprojectile::FindValidSpawnPosition()
{
    #define BOUNDSIZE 24
    #define CHECKPOSITION_COUNT 27
    idVec3 checkPositions[] =
    {
        idVec3(0,0,1),
        idVec3(0,1,1),
        idVec3(0,-1,1),

        idVec3(1,0,1),
        idVec3(1,1,1),
        idVec3(1,-1,1),

        idVec3(-1,0,1),
        idVec3(-1,1,1),
        idVec3(-1,-1,1),

        idVec3(0,0,0),
        idVec3(0,1,0),
        idVec3(0,-1,0),

        idVec3(1,0,0),
        idVec3(1,1,0),
        idVec3(1,-1,0),

        idVec3(-1,0,-1),
        idVec3(-1,1,-1),
        idVec3(-1,-1,-1),

        idVec3(0,0,-1),
        idVec3(0,1,-1),
        idVec3(0,-1,-1),

        idVec3(1,0,-1),
        idVec3(1,1,-1),
        idVec3(1,-1,-1),

        idVec3(-1,0,-1),
        idVec3(-1,1,-1),
        idVec3(-1,-1,-1),
    };

    //Get our directions of forward, right, up, relative to the surface normal.
    idVec3 forward, right, up;
    collisionNormal.ToAngles().ToVectors(&forward, &right, &up);

    idVec3 startPos = GetPhysics()->GetOrigin();
    idBounds bounds = idBounds(idVec3(-BOUNDSIZE, -BOUNDSIZE, -BOUNDSIZE), idVec3(BOUNDSIZE, BOUNDSIZE, BOUNDSIZE));
    for (int i = 0; i < CHECKPOSITION_COUNT; i++)
    {
        trace_t boundTr;
        idVec3 candidatePos = startPos + (forward * (checkPositions[i].z * BOUNDSIZE)) + (right * (checkPositions[i].x * BOUNDSIZE)) + (up * (checkPositions[i].y * BOUNDSIZE));
        gameLocal.clip.TraceBounds(boundTr, candidatePos, candidatePos, bounds, MASK_SHOT_RENDERMODEL, this);
        if (boundTr.fraction >= 1)
        {
            return candidatePos;
        }
    }

    return vec3_zero;
}
