#include "framework/DeclEntityDef.h"

#include "SmokeParticles.h"
#include "Fx.h"
#include "bc_asteroid.h"


const int ROTATIONSPEED = 4;
const int ROTATIONSPEEDMAX = 24;

const int PUSHAWAY_FORCE = 512;
const int DESPAWNTIME = 700;
const int SPAWNTIME = 2000;


#define THRUST_UPDATETIME 900


CLASS_DECLARATION(idMoveable, idAsteroid)
	EVENT(EV_Touch, idAsteroid::Event_Touch)
END_CLASS


idAsteroid::idAsteroid(void)
{
}

idAsteroid::~idAsteroid(void)
{
	StopSound(SND_CHANNEL_ANY, false);
}

void idAsteroid::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); //  asteroid_state_t state

	savefile->WriteInt( timer ); //  int timer

	savefile->WriteString( damageDef ); //  idString damageDef

	savefile->WriteInt( angularSpeed ); //  int angularSpeed
	savefile->WriteInt( moveSpeed ); //  int moveSpeed

	savefile->WriteParticle( trailParticles ); // const  idDeclParticle *	 trailParticles
	savefile->WriteInt( trailParticlesFlyTime ); //  int trailParticlesFlyTime

	savefile->WriteInt( despawnThreshold ); //  int despawnThreshold
}

void idAsteroid::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( (int&)state ); //  asteroid_state_t state

	savefile->ReadInt( timer ); //  int timer

	savefile->ReadString( damageDef ); //  idString damageDef

	savefile->ReadInt( angularSpeed ); //  int angularSpeed
	savefile->ReadInt( moveSpeed ); //  int moveSpeed

	savefile->ReadParticle( trailParticles ); // const  idDeclParticle * trailParticles
	savefile->ReadInt( trailParticlesFlyTime ); //  int trailParticlesFlyTime

	savefile->ReadInt( despawnThreshold ); //  int despawnThreshold
}

void idAsteroid::Spawn( void )
{
	state = SPAWNING;
	fl.takedamage = true;
	isFrobbable = false;	
	timer = gameLocal.time + SPAWNTIME;
	StartSound("snd_moving", SND_CHANNEL_AMBIENT, 0, false, NULL);
	
	moveSpeed = spawnArgs.GetInt("speed", "96");
	angularSpeed = (int)idMath::Lerp(ROTATIONSPEED, ROTATIONSPEEDMAX, gameLocal.random.RandomFloat());
	
	damageDef = spawnArgs.GetString("def_damage", "damage_asteroid");

	//Give it random starting angle.
	idAngles randRotation = idAngles(gameLocal.random.RandomInt(300), gameLocal.random.RandomInt(300), gameLocal.random.RandomInt(300));
	GetPhysics()->SetAxis(randRotation.ToMat3());
	GetPhysics()->SetContents(CONTENTS_SOLID | CONTENTS_NOBEACONCLIP);
	UpdateVisuals();

	trailParticles = NULL;
	trailParticlesFlyTime = 0;

	physicsObj.SetFriction(0,0,0); // override idMoveable defaults, which put a bit of linear friction on it and cause the asteroid to drift to a halt

	despawnThreshold = spawnArgs.GetInt("despawnThreshold", "-3000"); // This should be passed in by the envirospawner under normal circumstances

	BecomeActive(TH_THINK);
}

void idAsteroid::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	Shatter();
}

//void idAsteroid::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//	idMoveable::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);
//}

void idAsteroid::Think( void )
{

	if (state == SPAWNING)
	{
		//Done spawning in. Continue to thrust state.
		state = MOVING;
		renderEntity.shaderParms[7] = 0;
		GetPhysics()->SetGravity(idVec3(0, 0, 0));

		if (moveSpeed != 0)
		{
			idVec3 force;
			force = idVec3(0, -moveSpeed, 0); //Move southward.
			this->GetPhysics()->SetLinearVelocity(force);
		}

		this->GetPhysics()->SetAngularVelocity(idVec3(angularSpeed, 0, 0));
						
		//Set up the smoke trail.
		const char *smokeName = spawnArgs.GetString("smoke_fly");
		trailParticlesFlyTime = 0;
		if (*smokeName != '\0')
		{
			trailParticles = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, smokeName));
			trailParticlesFlyTime = gameLocal.time;
		}
		
		UpdateVisuals();
	}
	else if (state == MOVING)
	{
		// Update fade lerp for the first X milliseconds
		if (gameLocal.time < timer)
		{
			float lerp = (timer - gameLocal.time) / (float)SPAWNTIME;
			renderEntity.shaderParms[7] = lerp;
		}

		//Update smoke trail.
		if (trailParticles != NULL && trailParticlesFlyTime)
		{
			idVec3 dir = idVec3(0, 1, 0);
			idVec3 moveDir = GetPhysics()->GetLinearVelocity();
			moveDir.NormalizeFast();
			idVec3 particlePos = GetPhysics()->GetOrigin() + (moveDir * -56); //Spawn the particle BEHIND the asteroid.
			if (!gameLocal.smokeParticles->EmitSmoke(trailParticles, trailParticlesFlyTime, gameLocal.random.RandomFloat(), particlePos, dir.ToMat3(), timeGroup))
			{
				trailParticlesFlyTime = gameLocal.time;
			}
		}


		if (GetPhysics()->GetOrigin().y < despawnThreshold)
		{
			BeginDespawn();
		}

		RunPhysics();
	}
	else if (state == DESPAWNING)
	{
		//has reached the other side of the map. Despawn.
		float lerp;
		if (gameLocal.time >= timer)
		{
			lerp = 1;
			state = DORMANT;
			this->Hide();
			PostEventMS(&EV_Remove, 0);
		}
		else
		{
			lerp = 1.0f - ((timer - gameLocal.time) / (float)DESPAWNTIME);
		}

		renderEntity.shaderParms[7] = lerp;
		UpdateVisuals();
		RunPhysics();
	}
	
	Present();
}

void idAsteroid::BeginDespawn(void)
{
	fl.takedamage = false;
	state = DESPAWNING;
	timer = gameLocal.time + DESPAWNTIME;
	this->GetPhysics()->SetContents(0);
}

bool idAsteroid::Collide(const trace_t &collision, const idVec3 &velocity)
{
	if (state == DORMANT)
		return true;

	if (state == MOVING)
	{
		if (collision.c.material)
		{
			if (collision.c.material->GetSurfaceFlags() >= 256)
			{
				//Hit the SKY. Despawn.
				BeginDespawn();
				return true;
			}
		}

		//Detect if it hit a damageable.
		if (collision.c.entityNum >= 0 && collision.c.entityNum <= MAX_GENTITIES - 2)
		{
			InflictDamage(gameLocal.entities[collision.c.entityNum]);
		}

		Shatter();
	}

	return true;
}

void idAsteroid::Event_Touch(idEntity *other, trace_t *trace)
{
	if (state == DORMANT)
		return;

	InflictDamage(other);

	Shatter();
}

void idAsteroid::InflictDamage(idEntity *other)
{
	if (!other->fl.takedamage)
		return;

	if (other->health > 0)
	{
		//If they're alive, then inflict damage on them.
		other->Damage(this, this, vec3_zero, damageDef, 1.0f, 0);
	}

	//Push it away.
	idVec3 pushDir;
	pushDir = other->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
	pushDir.Normalize();
	other->GetPhysics()->ApplyImpulse(0, other->GetPhysics()->GetAbsBounds().GetCenter(), pushDir * PUSHAWAY_FORCE * other->GetPhysics()->GetMass());
}

void idAsteroid::Shatter()
{
	if (state == DORMANT)
		return;

	
	if (gameLocal.InPlayerConnectedArea(this)) //only do asteroid particle/sound if player can see it.
	{
		idEntityFx::StartFx(spawnArgs.GetString("fx_impact"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	}


	fl.takedamage = false;
	state = DORMANT;
	this->Hide();
	PostEventMS(&EV_Remove, 0);	
}
