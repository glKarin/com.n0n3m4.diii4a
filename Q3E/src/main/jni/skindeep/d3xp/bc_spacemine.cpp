#include "framework/DeclEntityDef.h"

#include "SmokeParticles.h"
#include "Fx.h"
#include "bc_spacemine.h"


const idVec3 IDLECOLOR = idVec3(1, .8f, 0);
const idVec3 WARNCOLOR = idVec3(1, .1f, .1f);
const idVec3 PURSUECOLOR = idVec3(1, 0, 1);

const int IMPULSE_MAXTIME = 300;

const int PROXIMITY_CHECKTIME = 300; //how often it checks for things near it
const int WARNING_RADIUS = 220; //how close you can get before it detects you
const int WARNING_MAXCOUNT = 3; //how much warning beeps does it give

const int PROPEL_INTERVALTIME = 2000; //how often it propels.

const int PURSUIT_INITIALDELAY = 400; //when it's alerted, how long before it does its first propel

const int EXPLOSIONDELAYTIME = 800; //when it's triggered, delay before it explodes.



#define WIREWIDTH 1.5f



CLASS_DECLARATION(idMoveable, idSpaceMine)
	EVENT(EV_Touch, idSpaceMine::Event_Touch)
END_CLASS


idSpaceMine::idSpaceMine(void)
{
}

idSpaceMine::~idSpaceMine(void)
{
	RemoveHarpoon();
	StopSound(SND_CHANNEL_ANY, false);
}

void idSpaceMine::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); // spacemine_state_t state

	savefile->WriteInt( timer ); // int timer

	savefile->WriteInt( warningCounter ); // int warningCounter
	savefile->WriteInt( warningTimer ); // int warningTimer

	savefile->WriteString( damageDef ); // idString damageDef

	savefile->WriteInt( moveSpeed ); // int moveSpeed

	savefile->WriteParticle( trailParticles ); // const idDeclParticle * trailParticles
	savefile->WriteInt( trailParticlesFlyTime ); // int trailParticlesFlyTime

	savefile->WriteObject( pursuitTarget ); // idEntityPtr<idEntity> pursuitTarget

	savefile->WriteObject( wireOrigin ); // idBeam* wireOrigin
	savefile->WriteObject( wireTarget ); // idBeam* wireTarget
	savefile->WriteObject( harpoonModel ); // idEntity * harpoonModel
	savefile->WriteBool( harpoonActive ); // bool harpoonActive

	savefile->WriteVec3( impulseTarget ); // idVec3 impulseTarget
	savefile->WriteBool( impulseActive ); // bool impulseActive
	savefile->WriteInt( impulseTimer ); // int impulseTimer
}

void idSpaceMine::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( (int&)state ); // spacemine_state_t state

	savefile->ReadInt( timer ); // int timer

	savefile->ReadInt( warningCounter ); // int warningCounter
	savefile->ReadInt( warningTimer ); // int warningTimer

	savefile->ReadString( damageDef ); // idString damageDef

	savefile->ReadInt( moveSpeed ); // int moveSpeed

	savefile->ReadParticle( trailParticles ); // const idDeclParticle * trailParticles
	savefile->ReadInt( trailParticlesFlyTime ); // int trailParticlesFlyTime

	savefile->ReadObject( pursuitTarget ); // idEntityPtr<idEntity> pursuitTarget

	savefile->ReadObject( CastClassPtrRef(wireOrigin) ); // idBeam* wireOrigin
	savefile->ReadObject( CastClassPtrRef(wireTarget) ); // idBeam* wireTarget
	savefile->ReadObject( harpoonModel ); // idEntity * harpoonModel
	savefile->ReadBool( harpoonActive ); // bool harpoonActive

	savefile->ReadVec3( impulseTarget ); // idVec3 impulseTarget
	savefile->ReadBool( impulseActive ); // bool impulseActive
	savefile->ReadInt( impulseTimer ); // int impulseTimer
}

void idSpaceMine::Spawn( void )
{
	state = IDLE;
	SetColor(IDLECOLOR);
	fl.takedamage = true;
	isFrobbable = false;	
	timer = 500;
	StartSound("snd_moving", SND_CHANNEL_AMBIENT, 0, false, NULL);
	GetPhysics()->SetGravity(idVec3(0, 0, 0));
	
	
	moveSpeed = spawnArgs.GetInt("speed", "96");
	
	
	damageDef = spawnArgs.GetString("def_damage", "damage_asteroid");

	//Give it random starting angle.
	//idAngles randRotation = idAngles(gameLocal.random.RandomInt(300), gameLocal.random.RandomInt(300), gameLocal.random.RandomInt(300));
	//GetPhysics()->SetAxis(randRotation.ToMat3());
	GetPhysics()->SetContents(CONTENTS_SOLID | CONTENTS_NOBEACONCLIP);
	UpdateVisuals();

	trailParticles = NULL;
	trailParticlesFlyTime = 0;

	physicsObj.SetFriction(0,0,0); // override idMoveable defaults, which put a bit of linear friction on it and cause the asteroid to drift to a halt

	team = spawnArgs.GetInt("team", "1"); //enemy team default
	warningCounter = 0;
	warningTimer = 0;
	pursuitTarget = NULL;

	harpoonModel = NULL;
	wireOrigin = NULL;
	wireTarget = NULL;
	harpoonActive = false;
	impulseTarget = vec3_zero;
	impulseActive = false;
	impulseTimer = 0;

	PostEventMS(&EV_PostSpawn, 0);
	BecomeActive(TH_THINK);
}

void idSpaceMine::Event_PostSpawn(void)
{
	//Handle the harpoon stuff. We do it here to make sure the harpoon target point has spawned in and exists.

	if (targets.Num() <= 0)
	{
		gameLocal.Error("Spacemine '%s' is missing a harpoon target (should target a target_null).", GetName());
		return;
	}

	idVec3 directionToHarpoon = targets[0].GetEntity()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
	directionToHarpoon.Normalize();

	#define MAXHARPOONLENGTH 3000
	trace_t harpoonTr;
	gameLocal.clip.TracePoint(harpoonTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + (directionToHarpoon * MAXHARPOONLENGTH), MASK_SOLID, NULL);
	if (harpoonTr.fraction >= 1)
	{
		gameLocal.Error("Spacemine '%s' harpoon length is too long (%d)", GetName(), MAXHARPOONLENGTH);
		return;
	}

	idVec3 harpoonPos = harpoonTr.endpos;
	idAngles harpoonAngle = (harpoonPos - GetPhysics()->GetOrigin()).ToAngles();
	float harpoonLength = (harpoonPos - GetPhysics()->GetOrigin()).Length();

	//harpoon model.
	idDict args;
	args.Clear();
	args.Set("classname", "func_static");
	args.Set("model", spawnArgs.GetString("model_hook"));
	args.SetVector("origin", harpoonPos);
	args.SetMatrix("rotation", harpoonAngle.ToMat3());
	args.SetBool("solid", false);
	gameLocal.SpawnEntityDef(args, &harpoonModel);

	//Spawn the cable.
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetFloat("width", WIREWIDTH);
	this->wireTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	this->wireTarget->Bind(this, false);
	
	args.Clear();
	args.Set("target", wireTarget->name.c_str());
	args.SetBool("start_off", false);
	args.SetVector("origin", GetPhysics()->GetOrigin() + (directionToHarpoon * (harpoonLength - 8)));
	args.SetFloat("width", WIREWIDTH);
	args.Set("skin", spawnArgs.GetString("skin_wire"));
	this->wireOrigin = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	this->wireOrigin->Bind(harpoonModel, false);

	harpoonActive = true;
}

void idSpaceMine::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	DoExplodeDelay();
}

//void idSpaceMine::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//	idMoveable::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);
//}

void idSpaceMine::Think( void )
{
	if (state == IDLE)
	{
		//see if there's anyone near me
		
		if (gameLocal.time > timer)
		{
			timer = gameLocal.time + PROXIMITY_CHECKTIME;
			idEntity *targetEnt = ProximityCheck();
			if (targetEnt != NULL)
			{
				state = WARNING;
				SetColor(WARNCOLOR);
				UpdateVisuals();
				timer = gameLocal.time + PROXIMITY_CHECKTIME;
				warningCounter = 0;
				warningTimer = 0;
				StartSound("snd_warnhum", SND_CHANNEL_BODY3);
			}
		}
	}
	else if (state == WARNING)
	{
		WarningState();
	}
	else if (state == PURSUING)
	{
		if (impulseActive)
		{			
			//Decelerate.
			idVec3 moveDirection = (impulseTarget - GetPhysics()->GetOrigin());
			moveDirection.Normalize();
			GetPhysics()->ApplyImpulse(0, GetPhysics()->GetOrigin(), moveDirection * 64000);

			if (GetPhysics()->GetLinearVelocity().Length() <= 0 || gameLocal.time > impulseTimer)
			{				
				idVec3 moveVelocity = moveDirection * moveSpeed;

				GetPhysics()->SetLinearVelocity(moveVelocity);

				impulseActive = false;

				GetPhysics()->SetAngularVelocity(idVec3(8, 0, 16));
				StartSound("snd_propel", SND_CHANNEL_BODY);
				idEntity *particle = gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin());
				if (particle)
				{
					particle->Bind(this, false);
				}
			}
		}

		if (gameLocal.time > timer && pursuitTarget.IsValid())
		{
			timer = gameLocal.time + PROPEL_INTERVALTIME;
			impulseActive = true;
			impulseTarget = pursuitTarget.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter();
			impulseTimer = gameLocal.time + 500;
		}

		//Update smoke trail.
		if (trailParticles != NULL && trailParticlesFlyTime)
		{
			idVec3 dir = idVec3(0, 1, 0);
			idVec3 moveDir = GetPhysics()->GetLinearVelocity();
			moveDir.NormalizeFast();
			idVec3 particlePos = GetPhysics()->GetOrigin() + (moveDir * -16); //Spawn the particle BEHIND the asteroid.
			if (!gameLocal.smokeParticles->EmitSmoke(trailParticles, trailParticlesFlyTime, gameLocal.random.RandomFloat(), particlePos, dir.ToMat3(), timeGroup))
			{
				trailParticlesFlyTime = gameLocal.time;
			}
		}

		RunPhysics();
	}
	else if (state == EXPLOSIONDELAY)
	{
		if (gameLocal.time > timer)
		{
			Explode();
		}

		RunPhysics();
	}

	
	Present();
}

idEntity *idSpaceMine::ProximityCheck()
{
	int closestDistance = 9999999;
	idEntity *targetEnt = NULL;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || entity->team == this->team || !entity->IsActive() || entity->health <= 0)
			continue;

		if (entity == gameLocal.GetLocalPlayer() && (gameLocal.GetLocalPlayer()->fl.notarget || gameLocal.GetLocalPlayer()->noclip)) //ignore noclip player.
			continue;



		//Check if in same room.
		idLocationEntity* myLocation = gameLocal.LocationForEntity(this);
		idLocationEntity* targetLocation = gameLocal.LocationForEntity(entity);

		if (myLocation != nullptr && targetLocation != nullptr)
		{
			if (myLocation->entityNumber != targetLocation->entityNumber)
			{
				//Not in same room. ignore.
				continue;
			}
		}




		float distance = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).Length();		
		if (distance <= WARNING_RADIUS && distance < closestDistance)
		{
			closestDistance = distance;
			targetEnt = entity;
		}
	}

	return targetEnt;
}

void idSpaceMine::WarningState()
{
	if (timer > gameLocal.time)
		return;

	timer = gameLocal.time + PROXIMITY_CHECKTIME;

	warningTimer++;
	idEntity *targetEnt = ProximityCheck();
	if (targetEnt != NULL)
	{
		StartSound("snd_warn", SND_CHANNEL_BODY);
		warningCounter++;
		gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin());
	}

	if (warningTimer >= WARNING_MAXCOUNT)
	{
		StopSound(SND_CHANNEL_BODY3);

		if (warningCounter >= WARNING_MAXCOUNT)
		{
			state = PURSUING;
			SetColor(PURSUECOLOR);
			UpdateVisuals();
			StartSound("snd_deploy", SND_CHANNEL_BODY2);
			pursuitTarget = targetEnt;
			timer = gameLocal.time + PURSUIT_INITIALDELAY;

			//Set up the smoke trail.
			const char *smokeName = spawnArgs.GetString("smoke_fly");
			trailParticlesFlyTime = 0;
			if (*smokeName != '\0')
			{
				trailParticles = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, smokeName));
				trailParticlesFlyTime = gameLocal.time;
			}
			
			RemoveHarpoon();
			

			return;
		}
		else
		{
			state = IDLE;
			SetColor(IDLECOLOR);
			UpdateVisuals();
			return;
		}
	}
}

bool idSpaceMine::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (IsHidden())
		return true;

	if (state == PURSUING)
	{
		bool isSky = false;
		if (collision.c.material)
		{
			if (collision.c.material->GetSurfaceFlags() >= 256)
			{
				//collided with sky. Do nothing.
				isSky = true;
			}
		}
		
		//Explode
		if (!isSky)
		{
			DoExplodeDelay();
		}
	}

	return true;
}

void idSpaceMine::Event_Touch(idEntity *other, trace_t *trace)
{
	if (IsHidden())
		return;

	DoExplodeDelay();
}

void idSpaceMine::DoExplodeDelay()
{
	if (state == EXPLOSIONDELAY)
		return;

	fl.takedamage = false;
	state = EXPLOSIONDELAY;
	StartSound("snd_delay", SND_CHANNEL_BODY);
	timer = gameLocal.time + EXPLOSIONDELAYTIME;	
	gameLocal.DoParticle(spawnArgs.GetString("model_explosiondelay"), GetPhysics()->GetOrigin());

	GetPhysics()->SetAngularVelocity(idVec3(16, 0, 32));
}

void idSpaceMine::Explode()
{
	if (state == EXPLODED)
		return;

	idStr deathscript = spawnArgs.GetString("call_onDeath");
	if (deathscript.Length() > 0)
	{
		gameLocal.RunMapScriptArgs(deathscript.c_str(), this, this);
	}

	state = EXPLODED;
	gameLocal.RadiusDamage(GetPhysics()->GetAbsBounds().GetCenter(), this, this, this, this, spawnArgs.GetString("def_damage"));
	idEntityFx::StartFx(spawnArgs.GetString("fx_explode"), GetPhysics()->GetOrigin(), mat3_identity);
	this->Hide();
	PostEventMS(&EV_Remove, 0);
}

void idSpaceMine::RemoveHarpoon()
{
	if (!harpoonActive)
		return;

	harpoonActive = false;
	
	if (this->wireTarget) 
	{
		this->wireTarget->PostEventMS(&EV_Remove, 0);
		wireTarget = nullptr;
	}
	if (this->wireOrigin)
	{
		this->wireOrigin->PostEventMS(&EV_Remove, 0);
		wireOrigin = nullptr;
	}
	if (this->harpoonModel)
	{
		this->harpoonModel->PostEventMS(&EV_Remove, 0);
		harpoonModel = nullptr;
	}
}
