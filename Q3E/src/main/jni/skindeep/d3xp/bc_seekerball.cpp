#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_seekerball.h"


//Force applied when they spawn into the world
const float LAUNCH_MINVEL = 48.0f;
const float LAUNCH_MAXVEL = 192.0f;

//A little delay before they explode.
const int EXPLOSION_DELAYTIME = 500;

const int WANDER_TIME = 400; //time between wander 'think's
const int PURSUIT_TIME = 300; //when have target and pursuing, update at this time interval.
const int PURSUIT_TIME_IMMEDIATEDISTANCE = 100; //If in immediate distance, then update navigation more accurately

const float WANDER_JUMPPOWER = 56.0f;	//jump power when idle
const float PURSUIT_JUMPPOWER = 72.0f;	//jump power when pursuing

const float PURSUIT_JUMPPOWER_IMMEDIATEDISTANCE = 160.0f; //when in Immediatedistance, then jump power increases
const float PURSUIT_IMMEDIATEDISTANCE = 192.0f; //when XX close, then speed up.

const float TRIGGER_RADIUS = 56.0f; //explode if I get this close to target.

const int LIFETIME_COUNTDOWNTIME = 4000; //do a ticking sound warning when I'm about to despawn.

const int JUMP_PITCH = -60; //angle of jump.

CLASS_DECLARATION(idMoveableItem, idSeekerBall)
EVENT(EV_Touch, idSeekerBall::Event_Touch)
END_CLASS

idSeekerBall::idSeekerBall(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;	
}

idSeekerBall::~idSeekerBall(void)
{
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	StopSound(SND_CHANNEL_ITEM);
}

void idSeekerBall::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteObject( enemyTarget ); // idEntityPtr<idEntity> enemyTarget

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteInt( lifetimeState ); // int lifetimeState
	savefile->WriteInt( lifetimeTimer ); // int lifetimeTimer
}

void idSeekerBall::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadObject( enemyTarget ); // idEntityPtr<idEntity> enemyTarget

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( lifetimeState ); // int lifetimeState
	savefile->ReadInt( lifetimeTimer ); // int lifetimeTimer
}

void idSeekerBall::Spawn(void)
{
	//non solid, but can take damage.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	isFrobbable = false;
	state = SB_WANDERING;
	stateTimer = 0;
	team = spawnArgs.GetInt("team", "1");
	enemyTarget = NULL;

	if (team <= 0)
	{
		//friendly.
		SetColor(FRIENDLYCOLOR);
	}
	else
	{
		SetColor(ENEMYCOLOR);
	}

	//Do initial launch out of the grenade nozzle.
	idVec3 launchDir = spawnArgs.GetVector("launchdir");
	float randVel = 48.0f + gameLocal.random.RandomInt(LAUNCH_MAXVEL - LAUNCH_MINVEL);
	GetPhysics()->SetLinearVelocity(launchDir * randVel);

	lifetimeState = LIFETIME_IDLE;
	lifetimeTimer = gameLocal.time + (spawnArgs.GetFloat("lifetime", "30") * 1000) - LIFETIME_COUNTDOWNTIME;

	BecomeActive(TH_THINK);
}

//Determine if I'm touching an enemy.
void idSeekerBall::Event_Touch(idEntity *other, trace_t *trace)
{
	if (other == this || state == SB_EXPLOSIONDELAY)
		return;

	if (other->IsType(idActor::Type) && other->team != this->team)
	{
		//touched an enemy. Start the explosion sequence.
		StartExplosionSequence();
	}
}

void idSeekerBall::StartExplosionSequence()
{
	state = SB_EXPLOSIONDELAY;
	stateTimer = gameLocal.time + EXPLOSION_DELAYTIME;
	StartSound("snd_warn", SND_CHANNEL_VOICE);


	GetPhysics()->SetContents(0);

	//Jump up.
	GetPhysics()->SetLinearVelocity(idVec3(0, 0, 384));

	idEntity *particle = gameLocal.DoParticle(spawnArgs.GetString("model_warning"), GetPhysics()->GetOrigin());
	if (particle)
	{
		particle->Bind(this, false);
	}
}

void idSeekerBall::Think(void)
{	
	if (state == SB_WANDERING)
	{
		//No target. Just lookin around....
		if (gameLocal.time > stateTimer && GetPhysics()->HasGroundContacts())
		{
			idEntity *newEnemy = FindEnemyTarget();
			if (newEnemy != NULL)
			{
				//Enter pursuit mode.
				StopSound(SND_CHANNEL_VOICE);
				StartSound("snd_sighted", SND_CHANNEL_VOICE);
				state = SB_PURSUIT;
				enemyTarget = newEnemy;
			}
			else
			{
				stateTimer = gameLocal.time + WANDER_TIME;

				//randomly hop around...
				idAngles randAngle = idAngles(JUMP_PITCH, gameLocal.random.RandomInt(360), 0);
				idVec3 impulse = randAngle.ToForward();
				GetPhysics()->ApplyImpulse(0, GetPhysics()->GetOrigin(), impulse * WANDER_JUMPPOWER * GetPhysics()->GetMass());
			}
		}
	}
	else if (state == SB_PURSUIT)
	{
		//I have a target. Pursuing target.

		if (!enemyTarget.IsValid())
		{
			LostTarget();
			return;
		}

		if (enemyTarget.GetEntity()->health <= 0)
		{
			LostTarget();
			return;
		}

		float distToTarget = (enemyTarget.GetEntity()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
		if (distToTarget <= TRIGGER_RADIUS)
		{
			//If we're within trigger radius, then explode.
			StartExplosionSequence();
			return;
		}

		//ok. We have a target. Attempt to move toward it.
		if (gameLocal.time > stateTimer && GetPhysics()->HasGroundContacts())
		{
			//Do the jump.
			float yawToEnemy = (enemyTarget.GetEntity()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).ToYaw();
			idVec3 jumpImpulse = idAngles(JUMP_PITCH, yawToEnemy, 0).ToForward();
			jumpImpulse.NormalizeFast();
			float jumpPower = PURSUIT_JUMPPOWER;

			if (distToTarget < PURSUIT_IMMEDIATEDISTANCE)
			{
				//If we're close to target, then increase movement speed/accuracy.
				jumpPower = PURSUIT_JUMPPOWER_IMMEDIATEDISTANCE;
				stateTimer = gameLocal.time + PURSUIT_TIME_IMMEDIATEDISTANCE; //if in immediate distance, then we update our direction more frequently.
			}
			else
			{
				//Normal pursuit.
				stateTimer = gameLocal.time + PURSUIT_TIME;
			}

			GetPhysics()->ApplyImpulse(0, GetPhysics()->GetOrigin(), jumpImpulse * jumpPower * GetPhysics()->GetMass()); //Do the jump.

			//Occasionally do idle sound.
			if (gameLocal.random.RandomInt(4) <= 0)
			{
				StartSound("snd_pursuit_idle", SND_CHANNEL_VOICE2);
			}			
		}
	}
	else if (state == SB_EXPLOSIONDELAY)
	{
		//waiting to explode.......

		if (gameLocal.time >= stateTimer)
		{
			//KABOOM!!!!!!!!!!!!!!!!
			state = SB_DONE;
			idEntityFx::StartFx("fx/explosion", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
			gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, NULL, NULL, spawnArgs.GetString("def_damage"));
			PostEventMS(&EV_Remove, 0);
		}
	}


	//Update renderlight.
	if (headlightHandle != -1)
	{
		headlight.origin = GetPhysics()->GetOrigin()  + idVec3(0,0,3);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}
	else
	{
		idVec3 teamColor;
		this->GetColor(teamColor);

		headlight.shader = declManager->FindMaterial("lights/defaultProjectedLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 48.0f;
		headlight.shaderParms[0] = teamColor.x / 2.0f;
		headlight.shaderParms[1] = teamColor.y / 2.0f;
		headlight.shaderParms[2] = teamColor.z / 2.0f;
		headlight.shaderParms[3] = 1.0f;
		headlight.noShadows = true;
		headlight.isAmbient = false;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	}


	if (lifetimeState == LIFETIME_IDLE)
	{		
		if (gameLocal.time >= lifetimeTimer)
		{
			//lifetime expired. start countdown.
			lifetimeState = LIFETIME_COUNTDOWN;
			StartSound("snd_countdown", SND_CHANNEL_ITEM);
			lifetimeTimer = gameLocal.time + LIFETIME_COUNTDOWNTIME;
		}
	}
	else if (lifetimeState == LIFETIME_COUNTDOWN)
	{
		if (gameLocal.time >= lifetimeTimer)
		{
			//countdown done. Fizzle out.
			Fizzle();
			return;
		}
	}

	idMoveableItem::Think();
}

//Fizzles out, no explosion, removes self from world.
void idSeekerBall::Fizzle()
{
	if (state == SB_DONE)
		return;

	state = SB_DONE;
	StopSound(SND_CHANNEL_ITEM);
	StartSound("snd_fizzle", SND_CHANNEL_ITEM);
	idEntityFx::StartFx(spawnArgs.GetString("fx_death"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	PostEventMS(&EV_Remove, 0);
}

//I had a target but I lost it.
void idSeekerBall::LostTarget()
{
	idEntity *newEnemy = FindEnemyTarget();
	if (newEnemy != NULL)
	{
		StopSound(SND_CHANNEL_VOICE);
		StartSound("snd_sighted", SND_CHANNEL_VOICE);
		enemyTarget = newEnemy;
		return;
	}

	//No valid target. Return to wander mode.
	state = SB_WANDERING;
	stateTimer = gameLocal.time + WANDER_TIME;
	StopSound(SND_CHANNEL_VOICE);
	StartSound("snd_lost", SND_CHANNEL_VOICE);
}


//Try to find an enemy to pursue.
idEntity * idSeekerBall::FindEnemyTarget()
{
	idEntity* closestEntity = NULL;
	float closestDistanceSq = idMath::INFINITY;

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (entity->team == this->team || entity->team == TEAM_NEUTRAL || entity->health <= 0)
			continue;

		if (entity == gameLocal.GetLocalPlayer() && (gameLocal.GetLocalPlayer()->noclip || gameLocal.GetLocalPlayer()->fl.notarget))
			continue;

		idVec3 eyePos;
		if (entity->IsType(idActor::Type))
		{
			eyePos = static_cast<idActor *>(entity)->GetEyePosition();
		}
		else
		{
			eyePos = entity->GetPhysics()->GetOrigin();
		}

		//Check if ball can see enemy eye
		trace_t losTr;
		gameLocal.clip.TracePoint(losTr, GetPhysics()->GetOrigin(), eyePos, MASK_SOLID, this);
		if (losTr.fraction < 1)
			continue; //no LOS.

		float curDist = (eyePos - GetPhysics()->GetOrigin()).LengthSqr();
		if (curDist < closestDistanceSq)
		{
			closestEntity = entity;
		}
	}

	return closestEntity;
}



//bool idSeekerBall::Collide(const trace_t &collision, const idVec3 &velocity)
//{
//	return idMoveableItem::Collide(collision, velocity);
//}

//void idSeekerBall::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
//{
//	common->Printf("");
//}

//I received damage.
void idSeekerBall::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	//Some leniency logic. If we're a friendly ball and take damage from a fellow friendly ball, don't take friendly damage.
	//This ensures that all friendly balls are able to emerge from the grenade, without potentially getting immediately blown up via friendly fire.
	if (inflictor != NULL)
	{
		if (team == TEAM_FRIENDLY && inflictor->IsType(idSeekerBall::Type) && inflictor->team == TEAM_FRIENDLY)
		{
			return;
		}
	}

	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

	if (attacker == NULL || health > 0)
		return;

	

	if (attacker == gameLocal.GetLocalPlayer())
	{
		//if I take damage from player and become dead, then do a radius check and delete any nearby seekerballs.
		//This is to make it a little easier to manage the seekerballs. Since they're so small and squirrelly we
		//do some leniency "splash damage" to nearby seekerballs.
		#define SELFREMOVAL_RADIUS 128
		int entityCount;
		idEntity *entityList[MAX_GENTITIES];
		entityCount = gameLocal.EntitiesWithinRadius(this->GetPhysics()->GetOrigin(), SELFREMOVAL_RADIUS, entityList, MAX_GENTITIES);
		for (int i = 0; i < entityCount; i++)
		{
			idEntity *ent = entityList[i];

			if (!ent)
				continue;

			if (ent->IsHidden() || ent->health <= 0 || !ent->IsType(idSeekerBall::Type)) //only want dead bodies.
				continue;

			static_cast<idSeekerBall *>(ent)->Fizzle();
		}
	}
}