#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

#include "ai/AI.h"
#include "bc_mech.h"

#include "bc_bananapeel.h"


CLASS_DECLARATION(idMoveableItem, idBananaPeel)
END_CLASS

const float FLIP_VELOCITY_THRESHOLD = 8;

const int PROXIMITY_CHECKTIME = 100;
const int VERTICAL_PROX_DIST = 8; //vertical distance can't be too high. i.e. we don't want the banana to activate when target is on a different floor

#define ACTORBOUNCETIME		500
#define ACTORBOUNCE_SPEED	128

idBananaPeel::idBananaPeel(void)
{
	particleEmitter = NULL;
}

idBananaPeel::~idBananaPeel(void)
{
	if (particleEmitter != NULL) {
		particleEmitter->PostEventMS(&EV_Remove, 0);
		particleEmitter = nullptr;
	}
}

void idBananaPeel::Spawn(void)
{
	//GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	//GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = spawnArgs.GetBool("frobbable", "1");

	BecomeActive(TH_THINK);
	fl.takedamage = true;
	
	thinkTimer = gameLocal.time + 300; //deleteme

	flipuprightCheckDone = false;

	//gameLocal.DoParticle(spawnArgs.GetString("smoke_place"), GetPhysics()->GetOrigin(), idVec3(1,0,0));

	idStr ambientParticle = spawnArgs.GetString("smoke_ambient");
	if (ambientParticle.Length() > 0)
	{
		idDict args;
		args.Clear();
		args.Set("model", ambientParticle.c_str());
		args.SetVector("origin", GetPhysics()->GetOrigin());
		args.SetBool("start_off", false);
		particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
		particleEmitter->Bind(this, false);
	}
}

void idBananaPeel::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( thinkTimer ); //  int thinkTimer
	savefile->WriteBool( flipuprightCheckDone ); //  bool flipuprightCheckDone

	savefile->WriteObject( particleEmitter ); //  idFuncEmitter * particleEmitter

	savefile->WriteInt( actorBounceTimer ); //  int actorBounceTimer
}

void idBananaPeel::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( thinkTimer ); //  int thinkTimer
	savefile->ReadBool( flipuprightCheckDone ); //  bool flipuprightCheckDone

	savefile->ReadObject(CastClassPtrRef(particleEmitter)); //  idFuncEmitter * particleEmitter

	savefile->ReadInt( actorBounceTimer ); //  int actorBounceTimer
}

void idBananaPeel::Think(void)
{
	idMoveableItem::Think();

	//Don't do proximity check if I'm being held by the player.
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			return;
		}
	}

	float curVelocity = GetPhysics()->GetLinearVelocity().Length();
	//if ( GetPhysics()->IsAtRest() && !flipuprightCheckDone)
	if (curVelocity <= FLIP_VELOCITY_THRESHOLD && !flipuprightCheckDone)
	{
		flipuprightCheckDone = true;
	
		//we want the banana to flip itself upright. It looks weird when it's resting on its side.
		idAngles bananaAngle = GetPhysics()->GetAxis().ToAngles();
		if (idMath::Fabs(bananaAngle.pitch) > 40 || idMath::Fabs(bananaAngle.roll) > 40)
		{
			idAngles newBananaAngle = idAngles(0, bananaAngle.yaw, 0);
			SetAxis(newBananaAngle.ToMat3());
	
			//bounce it up a little. TODO: make this more robust so the banana does some safety checks to make sure it's in the world, and doesn't fall under the floor or something.
			GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 2));
			GetPhysics()->SetLinearVelocity(idVec3(0, 0, 4));
		}
	
	}

	//Proximity check.
	if (gameLocal.time >= thinkTimer && !IsHidden())
	{
		thinkTimer = gameLocal.time + PROXIMITY_CHECKTIME; //how often to do proximity check.

		for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
		{
			if (!entity ||  !entity->IsActive() || entity->health <= 0)
				continue;

			bool isFriendlySlip = (entity->team == TEAM_FRIENDLY);

			float distance = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).Length();
			int triggerRadius = isFriendlySlip ? spawnArgs.GetInt("trigger_radius", "8") :  spawnArgs.GetInt("trigger_radius_ai", "48");

			//make sure target is on the same altitude as banana. make sure the vertical delta isn't too high.
			float absVerticalDelta = idMath::Fabs(GetPhysics()->GetOrigin().z - entity->GetPhysics()->GetOrigin().z);
			if (absVerticalDelta > VERTICAL_PROX_DIST)
				continue;

			if (isFriendlySlip && entity->entityNumber == gameLocal.GetLocalPlayer()->entityNumber &&
				(gameLocal.GetLocalPlayer()->IsCrouching() || gameLocal.GetLocalPlayer()->GetPhysics()->GetLinearVelocity().Length() <= 1))
			{
				//if player is crouching or standing still, don't slip
				continue;
			}


			if (entity->IsType(idAI::Type))
			{
				idAI *aiEnt = static_cast<idAI*>(entity);
				if (aiEnt->GetAIMovetype() == MOVETYPE_FLY || aiEnt->IsType(idMech::Type))
				{
					//do not apply to flying ai, or the mech (as funny as that would be)
					continue;
				}
			}


			if (distance < triggerRadius)
			{
				//someone stepped on the banana peel.
				this->Hide();
				Damage(entity, entity, vec3_zero, "damage_1000", 1.0f, 0); //delete self.
				//TODO: make a splat decal on ground.


				bool playSlipSound = false;

				if (entity == gameLocal.GetLocalPlayer())
				{
					//Player slipped on banana peel
					gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);
					gameLocal.GetLocalPlayer()->Damage(this, this, vec3_zero, "melee_gunnerkick", 1.0f, INVALID_JOINT);

					idStr playerSlipMessage = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_slip"), this->displayName.c_str());
					gameLocal.GetLocalPlayer()->SetCenterMessage(playerSlipMessage);
					gameLocal.AddEventLog(playerSlipMessage, this->GetPhysics()->GetOrigin());

					if (!gameLocal.GetLocalPlayer()->IsJockeying())
					{
						playSlipSound = true;
					}
				}
				else
				{
					//AI slipped on banana peel
					if (entity->IsType(idAI::Type))
					{
						if (static_cast<idAI*>(entity)->aiState != AISTATE_JOCKEYED)
						{

							//turn toward banana peel's angle.
							//float newYaw = GetPhysics()->GetAxis().ToAngles().yaw;
							//static_cast<idAI *>(entity)->TurnToward(newYaw);

							//start stun state. the stun state determines what animation is played.
							idStr bananaPainDef = spawnArgs.GetString("def_bananapain");
							static_cast<idAI*>(entity)->StartStunState(bananaPainDef);
							gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_enemyslip"), entity->displayName.c_str(), this->displayName.c_str()), this->GetPhysics()->GetOrigin());

							gameLocal.voManager.SayVO(entity, "snd_vo_slipping", VO_CATEGORY_HITREACTION);



							playSlipSound = true;
						}
					}
				}

				if (playSlipSound)
				{
					idEntityFx::StartFx(spawnArgs.GetString("fx_fall"), GetPhysics()->GetOrigin() + idVec3(0, 0, 1), mat3_identity, NULL);
				}


				return;
			}			
		}
	}	
}

void idBananaPeel::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!fl.takedamage)
		return;

	fl.takedamage = false;
	Hide();
	PostEventMS(&EV_Remove, 0);
}

void idBananaPeel::JustPickedUp()
{
	flipuprightCheckDone = false;
}

//bool idBananaPeel::DoFrob(int index, idEntity * frobber)
//{
//	idMoveableItem::DoFrob(index, frobber);
//
//	return true;
//}

bool idBananaPeel::Collide(const trace_t& collision, const idVec3& velocity)
{
	//If it hits an actor, bounce off them (don't explode/kill the banana if it's thrown at an actor)
	if (gameLocal.entities[collision.c.entityNum]->IsType(idActor::Type))
	{
		if (gameLocal.time > actorBounceTimer)
		{
			//common->Printf("bounce %d\n", gameLocal.time);
			//gameRenderWorld->DebugArrow(colorGreen, collision.c.point, collision.c.point + (collision.c.normal * 64), 6, 60000);

			actorBounceTimer = gameLocal.time + ACTORBOUNCETIME;
			idVec3 bounceDir = collision.c.normal * ACTORBOUNCE_SPEED;
			GetPhysics()->SetLinearVelocity(bounceDir);
		}
		
		return false; //don't collide with actors. This prevents the weird situation where you throw a bananapeel at someone and the bananapeel explodes to death.
	}

	return idMoveableItem::Collide(collision, velocity);
}