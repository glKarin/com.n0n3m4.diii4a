//Custom code for the repairbot.

#include "sys/platform.h"
#include "script/Script_Thread.h"
#include "gamesys/SysCvar.h"
#include "Mover.h"
#include "Fx.h"
#include "bc_meta.h"
#include "Player.h"
#include "idlib/LangDict.h"

#include "bc_dozerhatch.h"
#include "bc_repairbot.h"

const int PROXIMITY_TIMER = 300; //Check every XX milliseconds to see if I'm close enough to do repair.
const int REPAIRBOT_DISTANCE_THRESHOLD = 200; //Start doing the repair when I'm this close to object.
const int REPAIRTIME = 2000; //How long repair takes.

const int DEATH_COUNTDOWNTIME = 3000; //How long death throes are.

const int PAINTIME = 800;

const int BLOCK_VO_COOLDOWN = 2500; //how long between "excuse me" vo lines

const int DESPAWNTIME = 20000; //After idle for XX time, attempt to move to hatch and despawn.
const float DESPAWN_THRESHOLDDISTANCE = 128; //during despawn, has to be XX distance to hatch to despawn.
const int DESPAWN_PROXIMITY_TIMER = 300;

const int DONE_VO_DELAYTIME = 1500;

CLASS_DECLARATION(idAI, idAI_Repairbot)	
	EVENT(EV_Touch, idAI_Repairbot::Event_Touch)
END_CLASS

//NOTE: getphysics()->getaxis() does NOT work for ai actors. Use viewAxis instead to get angle information.

idAI_Repairbot::idAI_Repairbot(void)
{
	memset(&selfglowLight, 0, sizeof(selfglowLight));
	selfglowlightHandle = -1;
}

idAI_Repairbot::~idAI_Repairbot(void)
{
	repairNode.Remove();

	if (selfglowlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(selfglowlightHandle);
	}
}

void idAI_Repairbot::Spawn(void)
{
	//Spawn headlamp.
	idDict lightArgs, args;
	jointHandle_t headJoint;
	idVec3 jointPos;
	idMat3 jointAxis;

	headJoint = animator.GetJointHandle("headlamp");
	this->GetJointWorldTransform(headJoint, gameLocal.time, jointPos, jointAxis);
	lightArgs.Clear();
	lightArgs.SetVector("origin", jointPos);
	lightArgs.Set("texture", "lights/light_repairbot");
	lightArgs.SetInt("noshadows", 0);
	lightArgs.SetInt("start_off", 0);
	lightArgs.Set("light_right", "0 0 192");
	lightArgs.Set("light_target", "768 0 0");
	lightArgs.Set("light_up", "0 192 0");
	headLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
	headLight->SetAxis(viewAxis);
	headLight->BindToJoint(this, headJoint, true);
	headLight->SetColor(spawnArgs.GetVec4("_color"));	


	args.Clear();
	args.Set("model", "repaircrosses.prt");
	args.Set("start_off", "1");
	repairParticles = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));	
	repairParticles->Hide();


	if (1)
	{
		//Spawn the talk particles at the center of me.
		jointHandle_t bodyJoint;
		idVec3 bodyPos;
		idMat3 bodyAxis;
		bodyJoint = animator.GetJointHandle("body");
		this->GetJointWorldTransform(bodyJoint, gameLocal.time, bodyPos, bodyAxis);
		args.Clear();
		args.Set("model", "sound_burst.prt");
		args.Set("start_off", "1");
		talkParticles = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
		talkParticles->SetOrigin(bodyPos);
		talkParticles->Bind(this, false);
	}
	


	repairPosition = vec3_zero;	
	updateTimer = 0;
	patrolCooldownTimer = 1000;
	blockVoTimer = 0;

	despawnTimer = 0;
	isMovingToDespawn = false;
	despawnHatch = NULL;
	despawnProximityTimer = 0;

	idVec3 forward = viewAxis.ToAngles().ToForward();
	selfglowLight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
	selfglowLight.pointLight = true;
	selfglowLight.lightRadius[0] = selfglowLight.lightRadius[1] = selfglowLight.lightRadius[2] = 48.0f;
	selfglowLight.shaderParms[0] = spawnArgs.GetVector("_color").x; // R
	selfglowLight.shaderParms[1] = spawnArgs.GetVector("_color").y; // G
	selfglowLight.shaderParms[2] = spawnArgs.GetVector("_color").z; // B
	selfglowLight.shaderParms[3] = 1.0f;
	selfglowLight.noShadows = true;
	selfglowLight.isAmbient = false;
	selfglowLight.axis = mat3_identity;
	selfglowLight.origin = GetPhysics()->GetOrigin() + (forward * 32);
	selfglowlightHandle = gameRenderWorld->AddLightDef(&selfglowLight);

	GotoIdle();

	if (gameLocal.time > 0)
	{
		gameLocal.AddEventLog("#str_def_gameplay_repairbot_deployed", GetPhysics()->GetOrigin());
	}
}

void idAI_Repairbot::Save(idSaveGame* savefile) const
{
	savefile->WriteObject( repairEnt ); // idEntityPtr<idEntity> repairEnt

	savefile->WriteObject( queuedRepairEnt ); // idEntityPtr<idEntity> queuedRepairEnt

	savefile->WriteVec3( repairPosition ); // idVec3 repairPosition
	savefile->WriteInt( updateTimer ); // int updateTimer

	savefile->WriteObject( headLight ); // idLight * headLight
	savefile->WriteObject( repairParticles ); // idFuncEmitter * repairParticles

	savefile->WriteInt( patrolCooldownTimer ); // int patrolCooldownTimer

	savefile->WriteInt( blockVoTimer ); // int blockVoTimer

	savefile->WriteBool( isMovingToDespawn ); // bool isMovingToDespawn
	savefile->WriteInt( despawnTimer ); // int despawnTimer
	savefile->WriteObject( despawnHatch ); // idEntityPtr<idEntity> despawnHatch
	savefile->WriteInt( despawnProximityTimer ); // int despawnProximityTimer

	savefile->WriteObject( talkParticles ); // idFuncEmitter * talkParticles

	savefile->WriteRenderLight( selfglowLight ); // renderLight_t selfglowLight
	savefile->WriteInt( selfglowlightHandle ); // int selfglowlightHandle
}
void idAI_Repairbot::Restore(idRestoreGame* savefile)
{
	savefile->ReadObject( repairEnt ); // idEntityPtr<idEntity> repairEnt

	savefile->ReadObject( queuedRepairEnt ); // idEntityPtr<idEntity> queuedRepairEnt

	savefile->ReadVec3( repairPosition ); // idVec3 repairPosition
	savefile->ReadInt( updateTimer ); // int updateTimer

	savefile->ReadObject( CastClassPtrRef(headLight) ); // idLight * headLight
	savefile->ReadObject( CastClassPtrRef(repairParticles) ); // idFuncEmitter * repairParticles

	savefile->ReadInt( patrolCooldownTimer ); // int patrolCooldownTimer

	savefile->ReadInt( blockVoTimer ); // int blockVoTimer

	savefile->ReadBool( isMovingToDespawn ); // bool isMovingToDespawn
	savefile->ReadInt( despawnTimer ); // int despawnTimer
	savefile->ReadObject( despawnHatch ); // idEntityPtr<idEntity> despawnHatch
	savefile->ReadInt( despawnProximityTimer ); // int despawnProximityTimer

	savefile->ReadObject( CastClassPtrRef(talkParticles) ); // idFuncEmitter * talkParticles

	savefile->ReadRenderLight( selfglowLight ); // renderLight_t selfglowLight
	savefile->ReadInt( selfglowlightHandle ); // int selfglowlightHandle
	if ( selfglowlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( selfglowlightHandle, &selfglowLight );
	}
}


void idAI_Repairbot::Think(void)
{
	idAI::Think();

	//update self-glow renderlight.
	if (selfglowlightHandle != -1)
	{
		//Note: I think (?) this ignores the monster's pitch ; so the light is sometimes kinda in a weird place. Fix this if this becomes a problem.
		idVec3 forward = viewAxis.ToAngles().ToForward();
		selfglowLight.origin = GetPhysics()->GetOrigin() + (forward * 32);
		gameRenderWorld->UpdateLightDef(selfglowlightHandle, &selfglowLight);
	}

	if (queuedRepairEnt.IsValid())
	{
		if (SetRepairTask(queuedRepairEnt.GetEntity()))
		{
			queuedRepairEnt = NULL;
		}
	}

	if (aiState == REPAIRBOT_DEATHCOUNTDOWN)
	{
		//oh no I'm about to blow up

		if (gameLocal.time > updateTimer)
		{
			BlowUp();
		}
	}
	else if (aiState == REPAIRBOT_SEEKINGREPAIR)
	{
		if (!repairEnt.IsValid())
		{
			GotoIdle(); //Uh. the repair ent isn't valid for some reason. Kick back to idle.
			return;
		}

		if (gameLocal.time > updateTimer)
		{
			float distToRepairPos;

			updateTimer = gameLocal.time + PROXIMITY_TIMER;

			
			distToRepairPos = (repairPosition - this->GetPhysics()->GetOrigin()).LengthFast();

			if (distToRepairPos <= REPAIRBOT_DISTANCE_THRESHOLD)
			{
				//We are now close enough to do repair. Start repairing.

				int repairTime = REPAIRTIME;
				if (repairEnt.IsValid())
				{
					repairTime = repairEnt.GetEntity()->spawnArgs.GetInt("repairtime", "2000");
				}

				updateTimer = gameLocal.time + repairTime;
				aiState = REPAIRBOT_REPAIRING;

				talkParticles->SetActive(true);
				StartSound("snd_beam", SND_CHANNEL_BODY, 0, false, NULL);
				StartSound("snd_vo_repairing", SND_CHANNEL_VOICE, 0, false, NULL);

				//Lock the bot to be stationary and then make it turn toward the repairent.

				//lock the laser position.
				Event_SetLaserEndLock(repairPosition);

				//make turn rate very fast.
				Event_SetTurnRate(200);

				//disable move speed.
				Event_SetFlySpeed(.1f);			

				//disable movement.
				Event_AllowMovement(0);

				//Get direction to repairposition and face it.
				Event_TurnToPos(repairPosition);

				repairParticles->SetOrigin(repairPosition);
				repairParticles->Show();
				repairParticles->SetActive(true);
			}
		}
	}
	else if (aiState == REPAIRBOT_REPAIRING)
	{
		if (!repairEnt.IsValid())
		{
			GotoIdle(); //Uh. the repair ent isn't valid for some reason. Kick back to idle.
			return;
		}

		//Is currently repairing. Wait for the repair entity to be fully repaired.
		if (gameLocal.time > updateTimer)
		{
			//Done repairing.
			if (repairEnt.IsValid())
			{
				repairEnt.GetEntity()->DoRepairTick(0);
				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_repairbot_repaired"), repairEnt.GetEntity()->displayName.c_str()), repairEnt.GetEntity()->GetPhysics()->GetOrigin());
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointRepaired(repairEnt.GetEntity());
			}
			
			StopMove(MOVE_STATUS_DONE); //When done repairing, stop moving toward the repair ent.
			GotoIdle();

			//Particle fx.
			talkParticles->SetActive(true);
			idEntityFx::StartFx("fx/music_big", GetPhysics()->GetOrigin() + idVec3(0,0,16), mat3_identity);
			StartSound("snd_jingle", SND_CHANNEL_BODY, 0, false, NULL);
			StartSound("snd_vo_done", SND_CHANNEL_VOICE, 0, false, NULL);

			
			//BC 3-4-2025: add a delay before seeking next repair object, so that the "done" vo has time to play.
			updateTimer = gameLocal.time + DONE_VO_DELAYTIME;


			return;
		}
	}
	else if (aiState == REPAIRBOT_PAIN)
	{
		if (gameLocal.time > updateTimer)
		{
			this->SetColor(spawnArgs.GetVec4("_color"));

			if (repairEnt.IsValid())
			{
				SetRepairTask(repairEnt.GetEntity());
			}
			else
			{
				GotoIdle();
			}
		}
	}
	else if (aiState == REPAIRBOT_IDLE)
	{
		//Just tootle around to the patrol nodes

		//If I'm not moving, then give me somewhere new to move to.
		if (move.moveStatus != MOVE_STATUS_MOVING && gameLocal.time > patrolCooldownTimer && gameLocal.repairpatrolEntities.Num() > 0 && !isMovingToDespawn)
		{
			idEntity* nextNode;
			int randomIdx;

			randomIdx = gameLocal.random.RandomInt(gameLocal.repairpatrolEntities.Num());
			nextNode = gameLocal.repairpatrolEntities[randomIdx];

			if (nextNode != NULL)
			{
				MoveToPosition(nextNode->GetPhysics()->GetOrigin());
			}
			else
			{
				//If fail to move to patrol node, then just stop moving. and then try a different node.
				patrolCooldownTimer = gameLocal.time + 2000;
				StopMove(MOVE_STATUS_DONE);
			}
		}


		if (gameLocal.time >= despawnTimer && !isMovingToDespawn)
		{
			//Find a hatch to move toward.
			despawnHatch = NULL;
			int closestDistance = 9999;
			idEntity *closestHatch = NULL;
			for (idEntity* entity = gameLocal.hatchEntities.Next(); entity != NULL; entity = entity->hatchNode.Next())
			{
				if (!entity)
					continue;

				if (entity->IsType(idDozerhatch::Type))
				{
					float distance = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
					if (distance < closestDistance)
					{
						closestDistance = (int)distance;
						closestHatch = entity;
					}
				}
			}

			if (closestHatch != NULL)
			{
				talkParticles->SetActive(true);
				StartSound("snd_vo_despawn", SND_CHANNEL_VOICE, 0, false, NULL);
				despawnHatch = closestHatch;

				//Move to a space slightly in front of hatch.
				idVec3 upDir;
				closestHatch->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &upDir);
				MoveToPosition(closestHatch->GetPhysics()->GetOrigin() + upDir * -64);

				isMovingToDespawn = true;
			}			
		}
		else if (isMovingToDespawn && gameLocal.time >= despawnProximityTimer)
		{
			despawnProximityTimer = gameLocal.time + DESPAWN_PROXIMITY_TIMER;

			if (despawnHatch.IsValid())
			{
				if (ai_debugRepairbot.GetBool())
				{
					gameRenderWorld->DebugArrow(colorOrange, GetPhysics()->GetOrigin(), despawnHatch.GetEntity()->GetPhysics()->GetOrigin(), 8, DESPAWN_PROXIMITY_TIMER);
				}

				//Check if arrived at despawn hatch.
				float distanceToDespawnHatch = (despawnHatch.GetEntity()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
				if (distanceToDespawnHatch <= DESPAWN_THRESHOLDDISTANCE)
				{
					//Despawn.
					idEntityFx::StartFx("fx/smoke_ring09", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false); //Smoke particle.
					this->Hide();
					this->PostEventMS(&EV_Remove, 0);

					gameLocal.AddEventLog("#str_def_gameplay_repairbot_gohome", GetPhysics()->GetOrigin());

					//Attempt to play the animation on the hatch, if possible.
					if (!static_cast<idDozerhatch *>(despawnHatch.GetEntity())->IsCurrentlySpawning())
					{
						static_cast<idDozerhatch *>(despawnHatch.GetEntity())->Event_PlayAnim("close", 8);
						despawnHatch.GetEntity()->StartSound("snd_open", SND_CHANNEL_ANY);
					}
				}
			}
			else
			{
				//For some reason I am moving to despawn but do not have a valid despawn hatch. This shouldn't ever happen. Anyway, exit here and try to despawn again.
				isMovingToDespawn = false;
			}
		}
	}

	if (ai_showState.GetBool())
	{
		idStr stateName = "[INVALID]";
		switch (aiState)
		{
			case REPAIRBOT_IDLE: stateName = "IDLE"; break;
			case REPAIRBOT_SEEKINGREPAIR: stateName = "SEEKING REPAIR"; break;
			case REPAIRBOT_REPAIRING: stateName = "REPAIRING"; break;
			case REPAIRBOT_DEATHCOUNTDOWN: stateName = "DEATHCOUNTDOWN"; break;
			case REPAIRBOT_PAIN: stateName = "PAIN"; break;
		}

		gameRenderWorld->DrawText(idStr::Format("state: %s", stateName.c_str()), this->GetEyePosition() + idVec3(0, 0, 24), 0.24f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		idStr moveStatus = "[INVALID]";
		switch (move.moveStatus)
		{
			case MOVE_STATUS_DONE: moveStatus = "DONE"; break;
			case MOVE_STATUS_MOVING: moveStatus = "MOVING"; break;
			case MOVE_STATUS_WAITING: moveStatus = "WAITING"; break;
			case MOVE_STATUS_DEST_NOT_FOUND: moveStatus = "DEST NOT FOUND"; break;
			case MOVE_STATUS_DEST_UNREACHABLE: moveStatus = "DEST UNREACHABLE"; break;
			case MOVE_STATUS_BLOCKED_BY_WALL: moveStatus = "BLOCKED BY WALL"; break;
			case MOVE_STATUS_BLOCKED_BY_OBJECT: moveStatus = "BLOCKED BY OBJECT"; break;
			case MOVE_STATUS_BLOCKED_BY_ENEMY: moveStatus = "BLOCKED BY ENEMY"; break;
			case MOVE_STATUS_BLOCKED_BY_MONSTER: moveStatus = "BLOCKED BY MONSTER"; break;
		}

		gameRenderWorld->DrawText(idStr::Format("movestatus: %s", moveStatus.c_str()), this->GetEyePosition() + idVec3(0, 0, 16), 0.24f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		if (move.moveStatus != MOVE_STATUS_DONE && move.moveStatus != MOVE_STATUS_WAITING)
		{
			gameRenderWorld->DrawText(idStr::Format("dest: %.1f, %.1f, %.1f", move.moveDest.x, move.moveDest.y, move.moveDest.z), this->GetEyePosition() + idVec3(0, 0, 8), 0.2f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
			gameRenderWorld->DebugArrow(colorCyan, this->GetPhysics()->GetOrigin(), move.moveDest, 8, 1);
		}
	}

}

void idAI_Repairbot::GotoIdle()
{
	if (aiState == REPAIRBOT_DEATHCOUNTDOWN)
		return;

	SetSkin(declManager->FindSkin("skins/monsters/repairbot/default"));
	aiState = REPAIRBOT_IDLE;

	Event_SetLaserEndLock(vec3_zero);
	Event_SetTurnRate(spawnArgs.GetInt("turn_rate"));
	Event_SetFlySpeed(spawnArgs.GetInt("fly_speed"));
	Event_AllowMovement(1);

	StopSound(SND_CHANNEL_BODY, false);

	//Turn off repair particle fx.
	repairParticles->Hide();
	repairParticles->SetActive(false);

	repairEnt = NULL;
	repairPosition = vec3_zero;

	despawnTimer = gameLocal.time + DESPAWNTIME;
	isMovingToDespawn = false;
}


bool idAI_Repairbot::SetRepairTask(idEntity * thingToRepair)
{
	if (aiState == REPAIRBOT_DEATHCOUNTDOWN)
		return false;

	repairPosition = AttemptGoToRepairPoint(thingToRepair);	
	if (repairPosition == vec3_zero)
	{
		//gameLocal.Warning("'%s' failed to find route to repair '%s'\n", GetName(), thingToRepair->GetName());
		if (ai_debugRepairbot.GetBool())
		{
			gameRenderWorld->DebugArrow(colorRed, GetPhysics()->GetOrigin(), thingToRepair->GetPhysics()->GetOrigin(), 8, 100);
		}

		return false; //Move failed.
	}



	repairEnt = thingToRepair;
	aiState = REPAIRBOT_SEEKINGREPAIR;
	SetSkin(declManager->FindSkin("skins/monsters/repairbot/radiolines"));

	talkParticles->SetActive(true);
	StartSound("snd_vo_goal", SND_CHANNEL_VOICE, 0, false, NULL);
	isMovingToDespawn = false;

	return true;
}

idVec3 idAI_Repairbot::AttemptGoToRepairPoint(idEntity *ent)
{
	//Go to the entity's repair position.
	idVec3 _repairPos = ent->repairWorldposition;
	//gameRenderWorld->DebugArrowSimple(_repairPos);
	if (MoveToPosition(_repairPos))
	{
		//Success
		return _repairPos;
	}


	//Try to find a suitable place for the repairbot to move to.
	//We do some tracebound checks to find a clear spot nearby.
	const int ARRAYSIZE = 9;
	const int BOUNDSIZE = 24;
	//Forward, Right, Up
	idVec3 checkPositions[] = 
	{
		//Forward bottom row.
		idVec3(1,0,-1),
		idVec3(1,1,-1),
		idVec3(1,-1,-1),

		//Bottom row.
		idVec3(0,0,-1),
		idVec3(0,1,-1),
		idVec3(0,-1,-1),

		//Forward row.
		idVec3(1,0,0),
		idVec3(1,1,0),
		idVec3(1,-1,0),
	};

	idBounds bounds = idBounds(idVec3(-BOUNDSIZE, -BOUNDSIZE, -BOUNDSIZE), idVec3(BOUNDSIZE, BOUNDSIZE, BOUNDSIZE));
	idVec3 forward, right, up;
	ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	for (int i = 0; i < ARRAYSIZE; i++)
	{
		idVec3 candidatePosition = _repairPos + (forward * (checkPositions[i].x * BOUNDSIZE)) + (right * (checkPositions[i].y * BOUNDSIZE)) + (up * (checkPositions[i].z * BOUNDSIZE));
		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, candidatePosition, candidatePosition + idVec3(0,0,1), bounds, MASK_SOLID, NULL);
		if (boundTr.fraction >= 1.0f)
		{
			//Found a nice big empty spot to move to.
			if (ai_debugRepairbot.GetBool())
			{
				//gameRenderWorld->DebugArrowSimple(candidatePosition);
				gameRenderWorld->DebugBounds(colorRed, bounds, candidatePosition, 5000);
			}

			if (MoveToPosition(candidatePosition))
			{
				//Success
				return candidatePosition;
			}
		}
	}

	if (ai_debugRepairbot.GetBool())
	{
		#define REPAIRARROW_SHOWTIME 5000
		#define	REPAIRARROW_LENGTH 24
		gameRenderWorld->DebugArrow(colorRed, _repairPos + idVec3(0, 0, REPAIRARROW_LENGTH), _repairPos, 2, REPAIRARROW_SHOWTIME);
		gameRenderWorld->DrawText("Repairbot can't reach", _repairPos + idVec3(0, 0, REPAIRARROW_LENGTH), .2f, colorRed, mat3_default, 1, REPAIRARROW_SHOWTIME);
	}

	return vec3_zero; //total fail
}


void idAI_Repairbot::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	StopMove(MOVE_STATUS_DONE);
	Event_SetTurnRate(spawnArgs.GetInt("turn_rate"));
	Event_SetFlySpeed(spawnArgs.GetInt("fly_speed"));
	Event_AllowMovement(1);

	if (aiState == REPAIRBOT_DEATHCOUNTDOWN)
		return;

	idAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);

	if (health <= 0)
		return;

	talkParticles->SetActive(true);
	aiState = REPAIRBOT_PAIN;
	updateTimer = gameLocal.time + PAINTIME;
	this->SetColor(idVec4(1, 0, 0, 1)); //Turn red.
	StartSound("snd_pain", SND_CHANNEL_BODY, 0, false, NULL);
	StartSound("snd_vo_pain", SND_CHANNEL_VOICE, 0, false, NULL);
	repairParticles->Hide();
	repairParticles->SetActive(false);
	Event_SetLaserEndLock(vec3_zero);
}


void idAI_Repairbot::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (this->IsHidden() || aiState == REPAIRBOT_DEATHCOUNTDOWN)
		return;

	idAI::Killed(inflictor, attacker, damage, dir, location);

	repairParticles->Hide();
	repairParticles->SetActive(false);
	Event_SetLaserEndLock(vec3_zero);

	//If we're receiving crushing damage, then just make me immediately explode.
	if (inflictor->spawnArgs.GetBool("crusher", "0"))
	{
		BlowUp();
		return;
	}

	//Start the death logic.
	this->SetColor(idVec4(1, 0, 0, 1)); //Turn red.
	SetSkin(declManager->FindSkin("skins/monsters/repairbot/death"));	
	aiState = REPAIRBOT_DEATHCOUNTDOWN;
	updateTimer = gameLocal.time + DEATH_COUNTDOWNTIME;

	StopSound(SND_CHANNEL_AMBIENT, false);
	StopSound(SND_CHANNEL_VOICE, false);
	StopSound(SND_CHANNEL_BODY, false);
	StartSound("snd_glitch", SND_CHANNEL_BODY2, 0, false, NULL);

	
	//red glow particle fx.
	repairParticles->SetModel("repairbot_death.prt");
	repairParticles->SetOrigin(this->GetPhysics()->GetOrigin());
	repairParticles->Bind(this, false);
	repairParticles->SetActive(true);

}

void idAI_Repairbot::BlowUp()
{
	//Blow up

	if (this->IsHidden())
		return;

	idEntityFx::StartFx("fx/explosion_gascylinder", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);	
	Hide();
	GetPhysics()->SetContents(0);
	PostEventMS(&EV_Remove, 100);
	gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, "damage_explosion");
}

void idAI_Repairbot::Event_Touch(idEntity* other, trace_t* trace)
{
	if (!other->IsType(idActor::Type))
		return;

	if (gameLocal.time > blockVoTimer)
	{
		talkParticles->SetActive(true);
		blockVoTimer = gameLocal.time + BLOCK_VO_COOLDOWN;
		StartSound("snd_vo_block", SND_CHANNEL_VOICE, 0, false, NULL);
	}
}
