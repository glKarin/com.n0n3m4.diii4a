#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "Item.h"

#include "bc_wiregrenade.h"


const int PAYLOAD_TIMER = 90; //time between surface bounce + wire activation.
const int WIRELENGTH = 256;
const int WIRELENGTH_IMMEDIATEDEPLOY = 1024; // SW 3rd March 2025: Lowered this distance to something a bit less ridiculous

const float JUMP_POWER = 256;
const float WIREWIDTH = .3f;

const int TRIPDELAYTIME = 400; //time gap between tripping and exploding.

const int WIRETHINKTIME = 100;	//how often wire checks for targets ; its update time

const int LENIENCY_DISTANCE = 128; //When AI is XX distance from payload, then just blow up the payload. This is to make it easier for AI to trip the explosive.

const int PLAYER_LENIENCY_ARMTIME = 2000;	//after arming, have a grace period where player cannot trip it.

const float WIRE_ANGLE_THRESHOLD = .25f; //bias wire Z position to be within XX units. This is to make wires be more "flat" on the horizontal plane.

CLASS_DECLARATION(idMoveableItem, idWiregrenade)

END_CLASS

idWiregrenade::~idWiregrenade(void)
{
	// Carefully delete all our components before we destroy ourself
	for (int i = 0; i < MAXWIRES; i++)
	{
		if (this->wireTarget[i])
			delete this->wireTarget[i];
		
		if (this->wireOrigin[i])
			delete this->wireOrigin[i];
		
		if (this->wireHooks[i])
			delete this->wireHooks[i];
	}
}

void idWiregrenade::Spawn(void)
{
	idDict args;
	int i;

	hasBounced = false;
	grenadeTimer = 0;
	grenadeState = WIREGRENADE_IDLE;
	
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.Set("model", "model_wiregrenade");
	args.Set("dynamicSpectrum", this->dynamicSpectrum ? "1": "0");
	animatedEnt = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	animatedEnt->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	animatedEnt->Bind(this, true);

	for (i = 0; i < MAXWIRES; i++)
	{
		args.Clear();
		args.SetVector("origin", vec3_origin);
		args.SetFloat("width", WIREWIDTH);
		this->wireTarget[i] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);

		args.Clear();
		args.Set("target", wireTarget[i]->name.c_str());
		args.SetBool("start_off", true);
		args.SetVector("origin", vec3_origin);
		args.SetFloat("width", WIREWIDTH);
		args.Set("skin", "skins/beam_grenadewire");

		this->wireOrigin[i] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
		this->wireOrigin[i]->Hide();

		args.Clear();
		args.Set("model", "models/weapons/wiregrenade/wiregrenade_arm.ase");
		args.SetBool("solid", false);
		args.SetBool("noclipmodel", true);
		wireHooks[i] = (idStaticEntity*)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
		wireHooks[i]->Hide();
	}

	
	gameLocal.PrecacheModel(this, "mdl_projectile_cm");
	gameLocal.PrecacheModel(this, "mdl_hook");

	

	checkTimer = 0;
	
	this->fl.takedamage = true;

	if (spawnArgs.GetBool("deploy_immediately", "0"))
	{
		BecomeArmed(true);
	}

    playerLeniencyTime = gameLocal.time + PLAYER_LENIENCY_ARMTIME;
}

void idWiregrenade::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( hasBounced ); // bool hasBounced

	savefile->WriteInt( grenadeTimer ); // int grenadeTimer
	savefile->WriteInt( grenadeState ); // int grenadeState

	savefile->WriteObject( animatedEnt ); // idAnimated* animatedEnt

	SaveFileWriteArray( wireOrigin, MAXWIRES, WriteObject ); // idBeam* wireOrigin[MAXWIRES]
	SaveFileWriteArray( wireTarget, MAXWIRES, WriteObject ); // idBeam* wireTarget[MAXWIRES]

	SaveFileWriteArray( wireHooks, MAXWIRES, WriteObject ); // idStaticEntity* wireHooks[MAXWIRES]

	savefile->WriteInt( checkTimer ); // int checkTimer

	savefile->WriteInt( playerLeniencyTime ); // int playerLeniencyTime
}

void idWiregrenade::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( hasBounced ); // bool hasBounced

	savefile->ReadInt( grenadeTimer ); // int grenadeTimer
	savefile->ReadInt( grenadeState ); // int grenadeState

	savefile->ReadObject( CastClassPtrRef(animatedEnt) ); // idAnimated* animatedEnt

	SaveFileReadArrayCast( wireOrigin, ReadObject, idClass*& ); // idBeam* wireOrigin[MAXWIRES]
	SaveFileReadArrayCast( wireTarget, ReadObject, idClass*&  ); // idBeam* wireTarget[MAXWIRES]

	if (savefile->GetSaveVersion() <= SAVEGAME_VERSION_0002)
	{
		SaveFileReadArrayCast(wireTarget, ReadObject, idClass*&); // idStaticEntity* wireHooks[MAXWIRES] // this was accidentally set to wireTarget
	}
	else
	{
		SaveFileReadArrayCast(wireHooks, ReadObject, idClass*&); // idStaticEntity* wireHooks[MAXWIRES]
	}

	savefile->ReadInt( checkTimer ); // int checkTimer

	savefile->ReadInt( playerLeniencyTime ); // int playerLeniencyTime
}

void idWiregrenade::PostSaveRestore( idRestoreGame * savefile )
{
	if (savefile->GetSaveVersion() <= SAVEGAME_VERSION_0002)
	{
		// try to recover wires
		for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			if (ent->IsType(idStaticEntity::Type) && ent->GetPhysics() && (idStr::Icmp(ent->spawnArgs.GetString("model"), "models/weapons/wiregrenade/wiregrenade_arm.ase") == 0))
			{
				for (int idx = 0; idx < MAXWIRES; idx++)
				{
					if (!wireHooks[idx] && (ent != wireTarget[idx]) && (wireTarget[idx]->GetPhysics()->GetOrigin() == ent->GetPhysics()->GetOrigin()))
					{
						wireHooks[idx] = static_cast<idStaticEntity*>(ent);
						break;
					}
				}
			}
		}

		
		for (int idx = 0; idx < MAXWIRES; idx++)
		{
			if (!wireHooks[idx])
			{ 
				// couldn't recover, delete self
				BecomeInactive( TH_THINK & TH_PHYSICS & TH_UPDATEVISUALS );
				PostEventMS(&EV_Remove, 0);
				break;
			}
		}
	}
}

bool idWiregrenade::Collide(const trace_t &collision, const idVec3 &velocity)
{
	float		v;

	v = -(velocity * collision.c.normal);

	if (v < 1)
		return false;

	//BC 3-6-2025: ignore if I hit a sky brush.
	if (collision.c.material)
	{
		if (collision.c.material->GetSurfaceFlags() >= 256)
			return false; //It's sky. Exit here.
	}

	if (!hasBounced)
	{
        hasBounced = true;

        //if it touches an actor, explode.
        idEntity *ent = gameLocal.entities[collision.c.entityNum];
        if (ent)
        {
            if (ent->IsType(idAFEntity_Base::Type))
            {
                TripTheBomb();
                return false;
            }
        }


		//Jump upward from collision normal.
		//idVec3 fxPos = GetPhysics()->GetOrigin();
		//idMat3 fxMat = mat3_identity;
		idVec3 hitNormal = collision.c.normal;
		idVec3 facing = collision.c.normal;

		GetPhysics()->SetAngularVelocity( GetPhysics()->GetAngularVelocity() * .5f );
		

		hitNormal *= JUMP_POWER * GetPhysics()->GetMass();
		GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), hitNormal); //Jump off normal of wall.
		//GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), idVec3(0, 0, 384 * GetPhysics()->GetMass())); //Just jump upward, ignore normal.


		//facing.x += 90;
		GetPhysics()->SetAxis(facing.ToMat3());

		
		BecomeArmed(false);
		
	}

	return idMoveableItem::Collide(collision, velocity);
}

void idWiregrenade::BecomeArmed(bool immediateDeploy)
{
	grenadeState = WIREGRENADE_HASBOUNCED;
	grenadeTimer = immediateDeploy ? 0 : gameLocal.time + PAYLOAD_TIMER; //Do we deploy wires immediately, or do we wait for a bit before shooting out the wires.
	BecomeActive(TH_THINK);
	animatedEnt->SetSkin(declManager->FindSkin("skins/weapons/wiregrenade/skin_withshadow"));
	animatedEnt->Event_PlayAnim("opening", 0);
	idEntityFx::StartFx("fx/smoke_ring01", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false); //Particle FX.		
	GetPhysics()->GetClipModel()->SetOwner(NULL); //So that player can shoot it.
}


void idWiregrenade::Think(void)
{
	//RunPhysics();	

	if (grenadeState == WIREGRENADE_ISARMED)
	{

		//Check if any wire is hitting anything.
		if (gameLocal.time > checkTimer)
		{
			trace_t test1;

			checkTimer = gameLocal.time + WIRETHINKTIME;
			for (int i = 0; i < MAXWIRES; i++)
			{
				//Do trace check.
				trace_t wireTr;
			
				if (wireOrigin[i]->IsHidden())
					continue;

				gameLocal.clip.TracePoint(wireTr, GetPhysics()->GetOrigin(), wireTarget[i]->GetPhysics()->GetOrigin(), MASK_SOLID | MASK_PLAYERSOLID, NULL);			
				if (wireTr.c.entityNum <= MAX_GENTITIES - 2 && wireTr.c.entityNum >= 0)
				{
                    //check if a wire has been tripped.
					if (!gameLocal.entities[wireTr.c.entityNum]->IsType(idWorldspawn::Type) && !gameLocal.entities[wireTr.c.entityNum]->IsType(idStaticEntity::Type))
					{
                        //playerLeniencyTime
                        if (wireTr.c.entityNum == gameLocal.GetLocalPlayer()->entityNumber && gameLocal.time < playerLeniencyTime)
                        {
                            //player has touched the wire during the playerLeniency time. Do nothing....
                        }
                        else
                        {
                            //Wire has been tripped.
                            TripTheBomb();

                            //set beam to vibrate.
                            wireOrigin[i]->SetSkin(declManager->FindSkin("skins/beam_grenadewire_vibrate"));

                            return;
                        }
					}
				}
			}

			if (!GetPhysics()->IsAtRest())
			{
				TripTheBomb();
				grenadeTimer = 0;
			}

			//To make it more lenient, do an extra check that just does a distance check to baddies.			
			if (ShouldDoLeniencyCheck()) //leniency check is ignored in the deployImmediately trap mode.
			{
				for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
				{
					if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0 || entity->team != TEAM_ENEMY)
						continue;

					//doesn't need to be exact; do a quick and dirty distance check...
					idVec3 enemyFeetPos = entity->GetPhysics()->GetOrigin();
                    idVec3 enemyHeadPos = entity->GetPhysics()->GetOrigin() + idVec3(0,0,70);

					idVec3 grenadePos = this->GetPhysics()->GetOrigin();
					if (idMath::DistanceBoxCheck(grenadePos.x, grenadePos.y, grenadePos.z, LENIENCY_DISTANCE, enemyFeetPos.x, enemyFeetPos.y, enemyFeetPos.z)
                        || idMath::DistanceBoxCheck(grenadePos.x, grenadePos.y, grenadePos.z, LENIENCY_DISTANCE, enemyHeadPos.x, enemyHeadPos.y, enemyHeadPos.z))
					{
						TripTheBomb();
					}
				}
			}
		}
	}	
	else if (grenadeState == WIREGRENADE_HASBOUNCED)
	{
		if (gameLocal.time >= grenadeTimer)
		{
			//DEPLOY THE WIRES.

			//shoot payload wires.
			int i;
			idVec3* pointsOnSphere;
			bool isUpsideDown = gameLocal.random.RandomInt(2);
			int successfulWireCount = 0;
			idVec3 hitPoints[MAXWIRES];
			idVec3 hitNormals[MAXWIRES];

			bool hasHitPerson = false; //if hook hits person, trip the bomb immediately.

			jointHandle_t grenadeJoint = animatedEnt->GetAnimator()->GetJointHandle("origin");

			grenadeState = WIREGRENADE_ISARMED;

			//Find points on a sphere.
			//pointsOnSphere = gameLocal.GetPointsOnSphere(MAXWIRES);
			pointsOnSphere = GetWirepointsOnSphere(MAXWIRES, GetPhysics()->GetOrigin());

			if (pointsOnSphere == NULL)
			{
				//Something has gone awry, so I'm just going to explode and get outta here
				TripTheBomb();
				grenadeTimer = 0; //explode immediately.
				return;
			}

			//Give the points some random variation.
			//for (i = 0; i < 3; i++)
			//{
			//	int k;
			//	for (k = 0; k < 3; k++)
			//	{
			//		pointsOnSphere[i][k] += gameLocal.random.CRandomFloat() * .5f;
			//	}
			//}

			for (i = 0; i < MAXWIRES; i++)
			{
				hitPoints[i] = vec3_zero;
			}

			for (i = 0; i < MAXWIRES; i++)
			{
				//Attempt to shoot out a wire.
				trace_t trWire;
				idVec3 wireDir;				

				if (isUpsideDown)
				{
					pointsOnSphere[i].z = -pointsOnSphere[i].z;
				}

				wireDir = pointsOnSphere[i];
				wireDir.Normalize();

                idEntity *entityForWireToIgnore = this;

				bool immediateDeployMode = false;
				if (spawnArgs.GetBool("deploy_immediately", "0") && i <= 0)
				{
					//If in deploy immediately mode (attach to wall), then hijack the first wire and force it to attempt to be perpendicular to attached surface.
					idVec3 deployAngle = spawnArgs.GetVector("deploy_angle"); //deploy_angle is set via wiregrenade script file.
					if (deployAngle != vec3_zero)
					{
						wireDir = deployAngle;
						immediateDeployMode = true;
                        entityForWireToIgnore = gameLocal.GetLocalPlayer();
						//gameRenderWorld->DebugArrow(colorOrange, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + wireDir * 64, 2, 10000);

						//Make the grenade itself face perpendicular to the surface.
						if (animatedEnt != nullptr)
						{
							idAngles spawnAngle = deployAngle.ToAngles();
							animatedEnt->SetAngles(spawnAngle);
						}
					}
				}

                int currentMaxWireLength = immediateDeployMode ? WIRELENGTH_IMMEDIATEDEPLOY : WIRELENGTH;
				gameLocal.clip.TracePoint(trWire, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + (wireDir * currentMaxWireLength), MASK_SOLID | MASK_SHOT_RENDERMODEL | CONTENTS_BODY, entityForWireToIgnore);

				//Don't touch sky.
				if ((trWire.c.material != NULL) && (trWire.c.material->GetSurfaceFlags() & SURF_NOIMPACT))
				{
					continue;
				}

				// Make sure wire hits a wall AND is not too short.
				// SW 3rd March 2025: Tripwire can never be too short
				if (trWire.fraction < 1.0f && (immediateDeployMode || trWire.fraction >= .1f))
				{

					//gameRenderWorld->DebugLine(colorRed, GetPhysics()->GetOrigin(), trWire.endpos, 10000);
					hitPoints[i] = trWire.endpos;
					hitNormals[i] = trWire.c.normal;

					if (gameLocal.entities[trWire.c.entityNum] != NULL && gameLocal.entities[trWire.c.entityNum]->IsType(idActor::Type))
					{
						hasHitPerson = true;
					}

					successfulWireCount++;
				}

				if (immediateDeployMode && successfulWireCount > 0)
				{
					i = MAXWIRES + 1; //Exit here; immediate deploy wire was successful
				}
			}


			if (spawnArgs.GetBool("deploy_immediately", "0") && successfulWireCount > 0)
			{
				//Finalize immediate deployment.
				DeployWires(hitPoints, hitNormals, grenadeJoint); //Spawn wires.
				checkTimer = gameLocal.time + 300;

				//Deactivate physics.
				this->GetPhysics()->SetAngularVelocity(vec3_zero);
				this->GetPhysics()->SetLinearVelocity(vec3_zero);
				GetPhysics()->PutToRest();
				isFrobbable = true;

				idMoveableItem::Think();
				return;
			}


			if (successfulWireCount <= 1)
			{
				trace_t ceilingTr, floorTr;

				//No good. Not enough wires touched a surface.
				//gameRenderWorld->DebugArrow(colorRed, GetPhysics()->GetOrigin() + idVec3(0,0,128), GetPhysics()->GetOrigin(), 4, 5000);

				//Force a wire to come out of top and bottom.

				for (i = 0; i < MAXWIRES; i++)
				{
					hitPoints[i] = vec3_zero;
				}

				//Ceiling wire.
				gameLocal.clip.TracePoint(ceilingTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0, 0, 4096), MASK_SOLID | MASK_PLAYERSOLID, this);

				if ((ceilingTr.c.material != NULL) && (ceilingTr.c.material->GetSurfaceFlags() & SURF_NOIMPACT)) { }
				else if (ceilingTr.fraction < 1)
				{
					SpawnWire(wireOrigin[0], wireTarget[0], wireHooks[0], ceilingTr.endpos, ceilingTr.c.normal, grenadeJoint);
				}

				gameLocal.clip.TracePoint(floorTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0, 0, -4096), MASK_SOLID | MASK_PLAYERSOLID, this);

				if ((floorTr.c.material != NULL) && (floorTr.c.material->GetSurfaceFlags() & SURF_NOIMPACT)) {}
				else if (floorTr.fraction < 1)
				{
					SpawnWire(wireOrigin[1], wireTarget[1], wireHooks[1], floorTr.endpos, floorTr.c.normal, grenadeJoint);
				}

				hasHitPerson = false;
			}
			else
			{
				// ENABLE THE WIRES.
				int wiresPointingUpCount = DeployWires(hitPoints, hitNormals, grenadeJoint);
				

				//If there are no wires pointing upward, then force a wire to go upward. This is to prevent wiregrenades looking like they're floating in mid-air.
				if (wiresPointingUpCount <= 0)
				{
					//Find a wire that isn't being used.
					int wireIndex = -1;

					for (i = 0; i < MAXWIRES; i++)
					{
						if (wireOrigin[i]->IsHidden())
						{
							wireIndex = i;
							i = 999; //break out of loop.
						}
					}

					if (wireIndex >= 0)
					{
						trace_t ceilingTr;

						gameLocal.clip.TracePoint(ceilingTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0,0,4096), MASK_SOLID | MASK_PLAYERSOLID, this);

						//Don't touch sky.
						if ((ceilingTr.c.material != NULL) && (ceilingTr.c.material->GetSurfaceFlags() & SURF_NOIMPACT))
						{
							//don't attach to sky.
						}
						else if (ceilingTr.fraction < 1)
						{
							SpawnWire(wireOrigin[wireIndex], wireTarget[wireIndex], wireHooks[wireIndex], ceilingTr.endpos, ceilingTr.c.normal, grenadeJoint);
						}
					}
				}
			}

			if (hasHitPerson)
			{
				TripTheBomb();
			}
			else
			{
				idEntityFx::StartFx("fx/smoke_ring06", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
				this->isFrobbable = true;
				animatedEnt->Event_PlayAnim("spring", 0);

				this->GetPhysics()->SetAngularVelocity(vec3_zero);
				this->GetPhysics()->SetLinearVelocity(vec3_zero);
				GetPhysics()->PutToRest();
			}
		}
	}
	else if (grenadeState == WIREGRENADE_HASTRIPPED)
	{
		if (gameLocal.time > grenadeTimer)
		{
			idVec3 fxPos = GetPhysics()->GetOrigin();
			idMat3 fxMat = mat3_identity;
			const char *splashDmgDef;

			// E X P L O D E !

			this->fl.takedamage = false;

			//Particle FX.
			idEntityFx::StartFx("fx/explosion_gascylinder", &fxPos, &fxMat, NULL, false);

			grenadeState = WIREGRENADE_HASEXPLODED;

			

			for (int i = 0; i < MAXWIRES; i++)
			{
				if (!wireHooks[i]->IsHidden())
				{
					wireHooks[i]->SetSkin(declManager->FindSkin("skins/weapons/wiregrenade/skin_noblink"));
				}

				if (!wireOrigin[i]->IsHidden())
				{
					idVec3 midPoint = (GetPhysics()->GetOrigin() + wireTarget[i]->GetPhysics()->GetOrigin()) / 2.0f;
					idEntityFx::StartFx("fx/floatywire", &midPoint, &fxMat, NULL, false);
				}				
			}

			//Explode.
			splashDmgDef = spawnArgs.GetString("def_splash_damage", "damage_explosion");
			if (splashDmgDef && *splashDmgDef)
			{
				gameLocal.RadiusDamage(GetPhysics()->GetAbsBounds().GetCenter(), this, NULL, this, this, splashDmgDef);
				gameLocal.ThrowShrapnel(GetPhysics()->GetAbsBounds().GetCenter(), "projectile_shrapnel", this);
			}

			gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), "interest_explosion");

			//Remove self.
			PostEventMS(&EV_Remove, 0);
		}
	}
	// SW: Because thrown objects slowly drift to a halt in a vacuum, 
	// it's possible to get a case where the wire grenade never progresses beyond the 'idle' state, 
	// unable to be interacted with or detonated.
	// We break out of this special case by destroying the grenade and creating the pickup again.
	else if (grenadeState == WIREGRENADE_IDLE && this->GetPhysics()->GetLinearVelocity().LengthFast() < THROWABLE_DRIFT_THRESHOLD)
	{
		idMoveableItem::TryRevertToPickup();
	}

	//Present();
	idMoveableItem::Think();
}

int idWiregrenade::DeployWires(idVec3 hitPoints[MAXWIRES], idVec3 hitNormals[MAXWIRES], jointHandle_t grenadeJoint)
{
	int wiresPointingUpCount = 0;
	StartSound("snd_zips", SND_CHANNEL_ANY, 0, false, NULL);
	for (int i = 0; i < MAXWIRES; i++)
	{
		if (!SpawnWire(wireOrigin[i], wireTarget[i], wireHooks[i], hitPoints[i], hitNormals[i], grenadeJoint))
		{
			wireOrigin[i]->Hide();
		}
		else
		{
			float vdot;
			idVec3 wireDir;
			//gameRenderWorld->DebugLine(colorRed, wireOrigin[i]->GetPhysics()->GetOrigin(), wireTarget[i]->GetPhysics()->GetOrigin(), 10000);

			wireDir = hitPoints[i] - GetPhysics()->GetOrigin();
			wireDir.NormalizeFast();
			vdot = DotProduct(idVec3(0, 0, 1), wireDir);

			//Keep track of how many wires are pointing upward.
			if (vdot > 0)
			{
				wiresPointingUpCount++;
			}
		}
	}

	return wiresPointingUpCount;
}

bool idWiregrenade::SpawnWire(idBeam *startBeam, idBeam *endBeam, idEntity *hook, idVec3 targetPos, idVec3 targetNormal, jointHandle_t grenadeJoint)
{
	idVec3 angToGrenade;
	idMat3 fxMat;

	if (targetPos == vec3_zero)
	{
		//No wire here.
		return false;
	}

	startBeam->GetPhysics()->SetOrigin(GetPhysics()->GetOrigin());
	endBeam->GetPhysics()->SetOrigin(targetPos);
	startBeam->BindToJoint(animatedEnt, grenadeJoint, false);
	startBeam->BecomeActive(TH_PHYSICS);
	startBeam->Show();
	startBeam->spawnArgs.SetBool( "start_off", false );

	angToGrenade = GetPhysics()->GetOrigin() - targetPos;
	angToGrenade.Normalize();

	hook->GetPhysics()->SetOrigin(targetPos);
	hook->GetPhysics()->SetAxis(angToGrenade.ToMat3());
	hook->Show();

	gameLocal.ProjectDecal(targetPos, -targetNormal, 8.0f, true, 32.0f, "textures/decals/hookcrack");

	fxMat = targetNormal.ToMat3();
	idEntityFx::StartFx("fx/wire_sparks", &targetPos, &fxMat, NULL, false);

	return true;
}

void idWiregrenade::TripTheBomb(void)
{	
	if (grenadeState == WIREGRENADE_HASTRIPPED)
		return;

	//Make it explode.
	grenadeState = WIREGRENADE_HASTRIPPED;
	StartSound("snd_trip", SND_CHANNEL_BODY, 0, false, NULL);	
	grenadeTimer = gameLocal.time + TRIPDELAYTIME;
	fl.takedamage = false;
}

void idWiregrenade::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

	//Take damage.

	if (health <= 0)
	{
		TripTheBomb();
		grenadeTimer = 0;
	}
}

void idWiregrenade::Killed(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location)
{
	TripTheBomb();
}

bool idWiregrenade::DoFrob(int index, idEntity * frobber)
{
	if (frobber == gameLocal.GetLocalPlayer())
	{
		idVec3 fxPos = GetPhysics()->GetOrigin();
		idMat3 fxMat = mat3_identity;

		//REmove self.
		grenadeState = WIREGRENADE_NEUTRALIZED;
		fl.takedamage = false;
		this->isFrobbable = false;
		PostEventMS(&EV_Remove, 0);

		//Give the player one ammo.	
		gameLocal.GetLocalPlayer()->GiveItem("weapon_wiregrenade");
		//gameLocal.GetLocalPlayer()->SetAmmoDelta("ammo_wiregrenade", 1);

		//Particle fx for picking up item.
		idEntityFx::StartFx("fx/pickupitem", &fxPos, &fxMat, NULL, false);

		gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);
	}
	else
	{
		TripTheBomb();
	}

	return true;
}

bool idWiregrenade::ShouldDoLeniencyCheck()
{
	//If it's normally thrown , then we DO want to do the leniency check.
	if (!spawnArgs.GetBool("deploy_immediately", "0"))
		return true;

	
	//if it's tripwire and is DIRECTLY on the ground or ceiling (pointing straight up or straight down), then we *do* want leniencycheck.
	//Because otherwise the AI basically never walks over the tripwire.
	idVec3 wireStart = wireOrigin[0]->GetPhysics()->GetOrigin();
	idVec3 wireEnd = wireTarget[0]->GetPhysics()->GetOrigin();
	idVec3 wireDir = wireEnd - wireStart;
	wireDir.Normalize();	
	if (wireDir.x == 0 && wireDir.y == 0 && (wireDir.z == 1 || wireDir.z == -1))
	{
		return true;
	}
	

	
	//Tripwire placed on wall (or not floor), so do NOT do leniency check.
	return false;
}



idVec3* idWiregrenade::GetWirepointsOnSphere(const int num, idVec3 startpoint)
{
	//We want to generate wiregrenade points that fit criteria:
	// - We want to bias wires to be on the horizontal plane. This is to make it more likely for baddies to trip the wires.
	// - We want to bias wires to be long, but not TOO long. And we want to avoid very short wires.
	// - We want to avoid hitting actors. We don't want a wire to be immediately tripped.


	//Start with a raw list of points on a sphere.
	#define RAWCOUNT 64
	idVec3 * rawSpherePoints = gameLocal.GetPointsOnSphere(RAWCOUNT);

	//Filter the points.
	idList<idVec3> finalpoints;
	idStaticList<vecSpot_t, RAWCOUNT> vecspots;
	for (int i = 0; i < RAWCOUNT; i++)
	{
		idVec3 candidatePos = rawSpherePoints[i];

		//Filter the points to be ones that are relatively "flat" on the horizontal plane.
		if (idMath::Fabs(candidatePos.z) > WIRE_ANGLE_THRESHOLD)
			continue;
		
		//gameRenderWorld->DebugArrow(colorGreen, startpoint, startpoint + rawSpherePoints[i] * 256, 4, 90000);

		//Do a trace line check. The traceline distance is the max distance of the wire.
		trace_t tr;
		gameLocal.clip.TracePoint(tr, startpoint, startpoint + rawSpherePoints[i] * WIRELENGTH, MASK_SOLID | MASK_SHOT_RENDERMODEL | CONTENTS_BODY, NULL);
		if (tr.fraction >= 1.0f)
			continue; //Didn't hit anything........ just a wire floating in the void. The room is probably too large. Skip this wire.

		//Check if it hit an actor.
		if (gameLocal.entities[tr.c.entityNum] != NULL)
		{
			if (gameLocal.entities[tr.c.entityNum]->IsType(idStaticEntity::Type) || gameLocal.entities[tr.c.entityNum]->IsType(idWorldspawn::Type))
			{				
			}
			else
			{
				continue; //It hit an entity of some kind. We don't want the wiregrenade to immediately explode when it deploys. Skip this wire.
			}
		}
				
		//Store the wire distance
		vecSpot_t		spot;
		spot.position = rawSpherePoints[i];
		spot.distance = (tr.endpos - startpoint).LengthSqr();

		vecspots.Append(spot);
	}

	//We now have a bunch of candidates. Sort them by distance.
	qsort((void *)vecspots.Ptr(), vecspots.Num(), sizeof(vecSpot_t), (int(*)(const void *, const void *))gameLocal.sortVecPoints_Farthest);


	//First: Get the wires that are longest.
	if (vecspots.Num() > num)
	{
		for (int i = 0; i < num - 1; i++)
		{
			finalpoints.Append(vecspots[i].position);
		}

		//Lastly: add the SHORTEST wire. This makes the wires look more balanced.
		finalpoints.Append(vecspots[vecspots.Num() - 1].position);
	}
	else
	{
		for (int i = 0; i < vecspots.Num(); i++)
		{
			finalpoints.Append(vecspots[i].position);
		}
	}

	if (finalpoints.Num() <= 0)
	{
		//Something has gone wrong. Exit.
		gameLocal.Warning("Wiregrenade: fatal error in creating wires.");
		return NULL;
	}

	if (finalpoints.Num() < num)
	{
		//We don't have enough valid wirepoints. Add some wires that are variations of the last point.
		int amountToAdd = num - finalpoints.Num();
		idVec3 finalPointPos = finalpoints[finalpoints.Num() - 1];

		for (int i = 0; i < amountToAdd; i++)
		{
			finalpoints.Append(finalPointPos + idVec3(gameLocal.random.CRandomFloat(), gameLocal.random.CRandomFloat(), gameLocal.random.CRandomFloat()));
		}
	}

	
	//gameRenderWorld->DebugArrow(colorGreen, startpoint, startpoint +  * 384, 4, 90000);
	//gameRenderWorld->DebugArrow(colorGreen, startpoint, startpoint + vecspots[1].position * 384, 4, 90000);
	//gameRenderWorld->DebugArrow(colorGreen, startpoint, startpoint + vecspots[2].position * 384, 4, 90000);
	//gameRenderWorld->DebugArrow(colorGreen, startpoint, startpoint + vecspots[vecspots.Num() - 1].position * 384, 4, 90000);

	idVec3* returnArray = new idVec3[num];
	for (int i = 0; i < num; i++)
	{
		returnArray[i] = finalpoints[i];
	}	
	return returnArray;
}