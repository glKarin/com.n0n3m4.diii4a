//Custom code for the repairbot.

#include "sys/platform.h"
#include "script/Script_Thread.h"
#include "gamesys/SysCvar.h"
#include "Mover.h"
#include "Fx.h"
#include "SmokeParticles.h"
#include "framework/DeclEntityDef.h"
#include "bc_meta.h"
#include "Player.h"
#include "bc_spearprojectile.h"
#include "bc_spearbot.h"

const int SPEARBOT_PINGINTERVAL = 6000;
const int SPEARBOT_PINGCHARGETIME = 3000;
const int SPEARBOT_SCANTIME = 4000;			//how long scan time is.
const int SPEARBOT_RAMWARNTIME = 1500;
const int SPEARBOT_SUSPICIONPIP_MAX = 10;	//how long before it 'perceives' target. 1 pip is gained per 100ms.
const int SPEARBOT_DEATHFANFARETIME = 800;

const float BEAMWIDTH = 64;
const float TARGETINGLINEWIDTH = 32;

const float LIGHT_RADIUS = 128;

const int DEATHTARGET_RADIUS = 256;

#define BEAM_SWAY_SPEED .00002f

CLASS_DECLARATION(idAI, idAI_Spearbot)
END_CLASS

//NOTE: getphysics()->getaxis() does NOT work for ai actors. Use viewAxis instead to get angle information.


idAI_Spearbot::idAI_Spearbot(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	petNode.SetOwner(this);
	petNode.AddToEnd(gameLocal.petEntities);
}

idAI_Spearbot::~idAI_Spearbot(void)
{
	petNode.Remove();

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	for (int i = 0; i < SCANBEAMS; i++)
	{
		delete beamOrigin[i];
		delete beamTarget[i];
	}

	delete targetinglineStart;
	delete targetinglineEnd;

	delete scanchargeParticles;
}

void idAI_Spearbot::Spawn(void)
{
	aiState = SPEARSTATE_WANDERING;
	stateTimer = gameLocal.time + SPEARBOT_PINGINTERVAL - SPEARBOT_PINGCHARGETIME;
	suspicionPips = 0;
	suspicionTimer = 0;
	ramTargetEnt = NULL;
	ramPosition = vec3_zero;
	beamTimer = 0;
	isplayingLockBuzz = false;

	jointHandle_t bodyJoint = animator.GetJointHandle("column1");
	idVec3 jointPos;
	idMat3 jointAxis;
	animator.GetJointTransform(bodyJoint, gameLocal.time, jointPos, jointAxis);
	jointPos = this->GetPhysics()->GetOrigin() + jointPos * this->GetPhysics()->GetAxis();

	//Spawn beams.
	for (int i = 0; i < SCANBEAMS; i++)
	{
		idDict args;
		args.Clear();
		args.SetVector("origin", vec3_origin);
		//args.SetFloat("width", BEAMWIDTH);
		this->beamTarget[i] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);

		args.Clear();
		args.Set("target", beamTarget[i]->name.c_str());
		args.SetBool("start_off", true);
		//args.SetVector("origin", vec3_origin);
		args.SetFloat("width", BEAMWIDTH);
		args.SetVector("_color", idVec3(.7f, .7f, .7f));
		args.Set("skin", "skins/beam_spearscanner");
		this->beamOrigin[i] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);		
		this->beamOrigin[i]->SetOrigin(jointPos);
		this->beamOrigin[i]->BindToJoint(this, bodyJoint, false);

		beamRandomAngles[i] = idVec3(gameLocal.random.CRandomFloat(), gameLocal.random.CRandomFloat(), gameLocal.random.CRandomFloat());
	}

	//Targeting line. This is the line that appears when it acquires a valid target.
	idDict targetlineArg;
	targetlineArg.Clear();
	targetlineArg.SetVector("origin", vec3_origin);
	targetinglineEnd = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &targetlineArg);

	targetlineArg.Clear();
	targetlineArg.Set("target", targetinglineEnd->name.c_str());
	targetlineArg.SetBool("start_off", true);
	targetlineArg.SetFloat("width", TARGETINGLINEWIDTH);
	targetlineArg.Set("skin", "skins/beam_spear_lockon");
	targetlineArg.SetVector("origin", jointPos);
	targetinglineStart = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &targetlineArg);
	targetinglineStart->BindToJoint(this, bodyJoint, false);
	

	

	idDict args;
	args.Clear();
	args.Set("model", "spearbot_scancharge.prt");
	args.Set("start_off", "1");
	scanchargeParticles = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	scanchargeParticles->SetOrigin(jointPos);
	scanchargeParticles->Bind(this, true);

	if (team == TEAM_FRIENDLY)
		SetColor(FRIENDLYCOLOR);
	else
		SetColor(ENEMYCOLOR);
}


void idAI_Spearbot::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteInt( suspicionPips ); // int suspicionPips
	savefile->WriteInt( suspicionTimer ); // int suspicionTimer

	savefile->WriteInt( beamTimer ); // int beamTimer

	savefile->WriteObject( ramTargetEnt ); // idEntityPtr<idEntity> ramTargetEnt
	savefile->WriteVec3( ramPosition ); // idVec3 ramPosition
	savefile->WriteVec3( ramDirection ); // idVec3 ramDirection

	SaveFileWriteArray( beamOrigin, SCANBEAMS, WriteObject ); // idBeam*	beamOrigin[SCANBEAMS]
	SaveFileWriteArray( beamTarget, SCANBEAMS, WriteObject ); // idBeam* beamTarget[SCANBEAMS]

	SaveFileWriteArray( beamOffsets, SCANBEAMS, WriteVec3 ); // idVec3 beamOffsets[SCANBEAMS]
	SaveFileWriteArray( beamRandomAngles, SCANBEAMS, WriteVec3 ); // idVec3 beamRandomAngles[SCANBEAMS]

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteObject( scanchargeParticles ); // idFuncEmitter * scanchargeParticles

	savefile->WriteObject( targetinglineStart ); // idBeam* targetinglineStart
	savefile->WriteObject( targetinglineEnd ); // idBeam* targetinglineEnd

	savefile->WriteBool( isplayingLockBuzz ); // bool isplayingLockBuzz
}
void idAI_Spearbot::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadInt( suspicionPips ); // int suspicionPips
	savefile->ReadInt( suspicionTimer ); // int suspicionTimer

	savefile->ReadInt( beamTimer ); // int beamTimer

	savefile->ReadObject( ramTargetEnt ); // idEntityPtr<idEntity> ramTargetEnt
	savefile->ReadVec3( ramPosition ); // idVec3 ramPosition
	savefile->ReadVec3( ramDirection ); // idVec3 ramDirection

	SaveFileReadArrayCast( beamOrigin, ReadObject, idClass*& ); // idBeam* beamOrigin[SCANBEAMS]
	SaveFileReadArrayCast( beamTarget, ReadObject, idClass*& ); // idBeam* beamTarget[SCANBEAMS]

	SaveFileReadArray( beamOffsets, ReadVec3 ); // idVec3 beamOffsets[SCANBEAMS]
	SaveFileReadArray( beamRandomAngles, ReadVec3 );  // idVec3 beamRandomAngles[SCANBEAMS]

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadObject( CastClassPtrRef(scanchargeParticles) ); // idFuncEmitter * scanchargeParticles

	savefile->ReadObject( CastClassPtrRef(targetinglineStart) ); // idBeam* targetinglineStart
	savefile->ReadObject( CastClassPtrRef(targetinglineEnd) ); // idBeam* targetinglineEnd

	savefile->ReadBool( isplayingLockBuzz ); // bool isplayingLockBuzz
}

void idAI_Spearbot::Think(void)
{
	idAI::Think();
	
	if (aiState == SPEARSTATE_WANDERING)
	{	
		if (move.moveStatus != MOVE_STATUS_MOVING)
		{
			//Hijack the repairbot patrol nodes.
			bool successfulMove = false;
			if (gameLocal.repairpatrolEntities.Num() > 0)
			{

				idEntity* nextNode;
				int randomIdx;

				randomIdx = gameLocal.random.RandomInt(gameLocal.repairpatrolEntities.Num());
				nextNode = gameLocal.repairpatrolEntities[randomIdx];

				if (nextNode != NULL)
				{
					successfulMove = MoveToPosition(nextNode->GetPhysics()->GetOrigin());
				}
			}
			
			if (!successfulMove)
				WanderAround();
		}

		if (gameLocal.time >= stateTimer)
		{
			aiState = SPEARSTATE_SCANCHARGE;
			stateTimer = gameLocal.time + SPEARBOT_PINGCHARGETIME;
			StartSound("snd_sonarcharge", SND_CHANNEL_VOICE);

			scanchargeParticles->SetActive(true);

			if (move.moveStatus != MOVE_STATUS_DONE)
			{
				StopMove(MOVE_STATUS_DONE);
			}
		}
	}
	else if (aiState == SPEARSTATE_SCANCHARGE)
	{
		//Charging up the sonar...
		if (gameLocal.time > stateTimer)
		{
			//start the laser light show.
			aiState = SPEARSTATE_SCANNING;
			stateTimer = gameLocal.time + SPEARBOT_SCANTIME;
			StopSound(SND_CHANNEL_VOICE, false);
			StartSound("snd_sonarfire", SND_CHANNEL_VOICE);			

			for (int i = 0; i < SCANBEAMS; i++)
			{
				beamOffsets[i] = GetPhysics()->GetOrigin() + idVec3(gameLocal.random.RandomInt(-64, 64), gameLocal.random.RandomInt(-64, 64), gameLocal.random.RandomInt(-64, 64));
			}
		}
	}
	else if (aiState == SPEARSTATE_SCANNING)
	{
		//Search for enemies that I can see.
		//Using its lasers to fictionally scan the world and find enemies. It's not actually using these beams.... the beams are just a visual effect.

		if (gameLocal.time >= suspicionTimer)
		{
			suspicionTimer = gameLocal.time + 100; //Do the perception check every XX milliseconds.
			idEntity *enemyEnt = Event_FindEnemyAI(0);
			if (enemyEnt != NULL)
			{
				//I have to spot the enemy for XX amount of time before I go berserk.
				suspicionPips++;

				if (!isplayingLockBuzz)
				{
					StartSound("snd_visible", SND_CHANNEL_WEAPON);
					isplayingLockBuzz = true;
				}

				if (suspicionPips >= SPEARBOT_SUSPICIONPIP_MAX)
				{
					//My suspicion has been maxed out. I go berserk.
					ramTargetEnt = enemyEnt;
					aiState = SPEARSTATE_RAMWARNING;
					stateTimer = gameLocal.time + SPEARBOT_RAMWARNTIME;
					StopSound(SND_CHANNEL_VOICE);
					StopSound(SND_CHANNEL_WEAPON);
					StartSound("snd_sighted", SND_CHANNEL_VOICE2);
					Event_SetFlyBobStrength(0);
					Event_SetTurnRate(2000);
					Event_TurnToPos(enemyEnt->GetPhysics()->GetAbsBounds().GetCenter());
					Event_LookAtPoint(enemyEnt->GetPhysics()->GetAbsBounds().GetCenter(), 3);

					//Turn off light.
					if (headlightHandle != -1)
					{
						gameRenderWorld->FreeLightDef(headlightHandle);
						headlightHandle = -1;
					}

					//Spawn interestpoint when I yell.
					gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_yell"));

					targetinglineEnd->SetOrigin(enemyEnt->GetPhysics()->GetOrigin());
					targetinglineStart->Show();
					return;
				}
			}
			else if (isplayingLockBuzz)
			{
				StopSound(SND_CHANNEL_WEAPON);
				isplayingLockBuzz = false;
			}
		}

		//Animate the beams.
		jointHandle_t bodyJoint = animator.GetJointHandle("body");
		idVec3 jointPos;
		idMat3 jointAxis;
		animator.GetJointTransform(bodyJoint, gameLocal.time, jointPos, jointAxis);
		jointPos = this->GetPhysics()->GetOrigin() + jointPos * this->GetPhysics()->GetAxis();
		for (int i = 0; i < SCANBEAMS; i++)
		{
			idVec3 beamDir = beamOffsets[i] - jointPos;
			beamDir += idVec3(idMath::Cos(beamOffsets[i].x * gameLocal.time * BEAM_SWAY_SPEED) * 40.0f, idMath::Cos(beamOffsets[i].y * gameLocal.time * BEAM_SWAY_SPEED) * 40.0f, idMath::Cos(beamOffsets[i].z * gameLocal.time * BEAM_SWAY_SPEED) * 40.0f);
			beamDir.NormalizeFast();
			

			//See where beam hits the world.
			trace_t beamTr;
			gameLocal.clip.TracePoint(beamTr, jointPos, jointPos + beamDir * 1024, MASK_SHOT_RENDERMODEL, NULL);
			this->beamTarget[i]->SetOrigin( beamTr.endpos);

			if (beamOrigin[i]->IsHidden())
				beamOrigin[i]->Show();			
		}

		if (headlightHandle <= -1)
		{
			// Create light source. We use a renderlight (i.e. not a real light entity) to prevent weird physics binding jitter
			headlight.shader = declManager->FindMaterial(spawnArgs.GetString("mtr_scanlight"), false); // SW 16th April 2025: made this data-driven
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHT_RADIUS;

			idVec3 lightColor = (team == TEAM_FRIENDLY) ? idVec3(FRIENDLYCOLOR.x/2.0f, FRIENDLYCOLOR.y / 2.0f, FRIENDLYCOLOR.z / 2.0f) : idVec3(.5f, .5f, .5f);
			headlight.shaderParms[0] = lightColor.x; // R
			headlight.shaderParms[1] = lightColor.y; // G
			headlight.shaderParms[2] = lightColor.z; // B

			headlight.shaderParms[3] = 1.0f; // ???
			headlight.noShadows = true;
			headlight.isAmbient = false;
			headlight.axis = mat3_identity;
			headlight.affectLightMeter = true; // SW 16th April 2025: Light should affect player if they're shadowy
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
		}

		if (gameLocal.time > stateTimer)
		{			
			if (isplayingLockBuzz)
			{
				//if bot currently HAS a target, then continue the scanning state.
				aiState = SPEARSTATE_SCANNING;
				stateTimer = gameLocal.time + SPEARBOT_SCANTIME;
				StartSound("snd_sonarfire", SND_CHANNEL_VOICE);
			}
			else
			{
				//bot does NOT have a target. Exit back to wander state.
				for (int i = 0; i < SCANBEAMS; i++)
				{
					beamOrigin[i]->Hide();
				}

				StopSound(SND_CHANNEL_WEAPON);
				aiState = SPEARSTATE_WANDERING;
				stateTimer = gameLocal.time + SPEARBOT_PINGINTERVAL - SPEARBOT_PINGCHARGETIME;

				//Turn off light.
				if (headlightHandle != -1)
				{
					gameRenderWorld->FreeLightDef(headlightHandle);
					headlightHandle = -1;
				}
			}
		}
	}
	else if (aiState == SPEARSTATE_RAMWARNING)
	{
		//Ram warning.
		if (ramTargetEnt.IsValid())
		{
			//Attempt to keep looking at the enemy ent.
			//Do LOS check.
			idVec3 targetPosition = GetTargetPosition(ramTargetEnt.GetEntity());
			if (targetPosition != vec3_zero)
			{
				//We have LOS. Update the ram/laser position.				
				ramPosition = targetPosition;
			}
		}

		//Update the targeting line.
		if (1)
		{
			//do traceline.
			trace_t targetinglineTr;
			idVec3 targetinglineDir = ramPosition - targetinglineStart->GetPhysics()->GetOrigin();
			targetinglineDir.NormalizeFast();
			gameLocal.clip.TracePoint(targetinglineTr, targetinglineStart->GetPhysics()->GetOrigin(), targetinglineStart->GetPhysics()->GetOrigin() + targetinglineDir * 1024, MASK_SHOT_RENDERMODEL, this);
			//gameRenderWorld->DebugArrow(colorGreen, targetinglineStart->GetPhysics()->GetOrigin(), targetinglineTr.endpos, 8, 100);
			targetinglineEnd->SetOrigin(targetinglineTr.endpos);
		}

		//make all the beams point toward ramposition.
		if (gameLocal.time > beamTimer)
		{
			beamTimer = gameLocal.time + 300;
			for (int i = 0; i < SCANBEAMS; i++)
			{
				beamOrigin[i]->SetColor(.4f, 0, 0); //make the beams go red-alert red.
				beamOrigin[i]->UpdateVisuals();
				beamTarget[i]->SetOrigin(ramPosition + idVec3(gameLocal.random.RandomInt(-128, 128), gameLocal.random.RandomInt(-128, 128), gameLocal.random.RandomInt(-128, 128)));
			}
		}

		if (gameLocal.time >= stateTimer)
		{
			aiState = SPEARSTATE_DONE;
			StopSound(SND_CHANNEL_ANY);

			//GetPhysics()->SetGravity(vec3_zero);

			//Find the direction we want to thrust toward.
			ramDirection = ramPosition - targetinglineStart->GetPhysics()->GetOrigin();
			ramDirection.Normalize();

			Launch();
		}
	}	
	else if (aiState == SPEARSTATE_DEATHFANFARE)
	{
		if (gameLocal.time >= stateTimer)
		{
			//If there's a baddie nearby, then magnetize toward them.
			idVec3 targetPosition = FindDeathTarget();
			if (targetPosition == vec3_zero)
			{
				//Just go straight downward.
				targetPosition = GetPhysics()->GetOrigin() + idVec3(1, 1, -512);
			}

			ramPosition = targetPosition;
			Launch();
		}
	}

	if (headlightHandle != -1)
	{
		headlight.origin = GetPhysics()->GetOrigin() + idVec3(0,0,-24);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}	
}

//If I'm about to perish, aim toward organic entities. because that's funnier
idVec3 idAI_Spearbot::FindDeathTarget()
{
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->GetPhysics())
			continue;

		idVec3 entityPos = entity->GetPhysics()->GetOrigin();
		idVec3 myPos = GetPhysics()->GetOrigin();

		if (entity == gameLocal.GetLocalPlayer() || entity == this || entityPos.z > myPos.z)
			continue;

		if (entityPos.x > myPos.x - DEATHTARGET_RADIUS && entityPos.x < myPos.x + DEATHTARGET_RADIUS && entityPos.y > myPos.y - DEATHTARGET_RADIUS && entityPos.y < myPos.y + DEATHTARGET_RADIUS)
		{
			//target is within the death killzone.
			//check LOS.
			idVec3 targetPosition = GetTargetPosition(entity);
			if (targetPosition != vec3_zero)
			{
				return targetPosition;
			}
		}
	}

	return vec3_zero;
}


//Is mad, launch it toward enemy.
void idAI_Spearbot::Launch()
{
	//Swap over to the projectile entity.
	StopSound(SND_CHANNEL_WEAPON);
	aiState = SPEARSTATE_DONE;
	this->Hide();
	this->PostEventMS(&EV_Remove, 0);

	//Spawn projectile entity. This is what shoots toward the enemy.
	idEntity *projectileEnt;
	idDict args;
	args.Set("classname", spawnArgs.GetString("def_spear"));
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetVector("rampoint", ramPosition);

	//Set owner of projectile.
	const char *owner = spawnArgs.GetString("owner");
	if (owner[0] != '\0')
	{
		args.Set("owner", owner);
	}

	gameLocal.SpawnEntityDef(args, &projectileEnt);
}

idVec3 idAI_Spearbot::GetTargetPosition(idEntity *targetEnt)
{
	for (int i = 0; i < 3; i++)
	{
		idVec3 targetPosition;

		//Check a few positions on the target.
		if (i == 0)
		{
			//Try to aim at center mass.
			targetPosition = targetEnt->GetPhysics()->GetOrigin() + idVec3(0,0,targetEnt->GetPhysics()->GetBounds()[1].z / 2);
		}
		else if (i == 1)
		{
			//Try to aim at head.
			targetPosition = targetEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, targetEnt->GetPhysics()->GetBounds()[1].z - 8 );
		}
		else
		{
			//Try to aim at feet.
			targetPosition = targetEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, 8);
		}

		trace_t losTr;
		gameLocal.clip.TracePoint(losTr, GetPhysics()->GetOrigin(), targetPosition, MASK_SOLID, NULL);
		if (losTr.fraction >= 1 || losTr.c.entityNum == targetEnt->entityNumber)
		{
			//Trace is good. Found valid spot.
			return targetPosition;
		}
	}

	//Trace failed.
	return vec3_zero;
}



void idAI_Spearbot::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);
}


void idAI_Spearbot::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (aiState == SPEARSTATE_DONE || aiState == SPEARSTATE_DEATHFANFARE)
		return;

	StopSound(SND_CHANNEL_WEAPON);
	stateTimer = gameLocal.time + SPEARBOT_DEATHFANFARETIME;
	aiState = SPEARSTATE_DEATHFANFARE;
	StartSound("snd_death", SND_CHANNEL_VOICE);

	scanchargeParticles->SetModel("smoke_sparks_03.prt");
	scanchargeParticles->SetActive(true);

	// SW 16th April 2025: Woah woah woah wait! Is our scanning light currently active? Make sure to shut that off first!!
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}

	//Yellow spark light.
	headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHT_RADIUS;
	headlight.shaderParms[0] = 0.5f; // R
	headlight.shaderParms[1] = 0.5f; // G
	headlight.shaderParms[2] = 0; // B
	headlight.shaderParms[3] = 1.0f; // ???
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);
}

void idAI_Spearbot::DoHack()
{
	gameLocal.AddEventLog("#str_def_gameplay_swordfish_hack", GetPhysics()->GetOrigin());

	team = TEAM_FRIENDLY;
	SetColor(FRIENDLYCOLOR);

	//If I was about to ram, then reset myself.
	if (aiState == SPEARSTATE_RAMWARNING)
	{
		aiState = SPEARSTATE_WANDERING;
		StopSound(SND_CHANNEL_VOICE2);
		StopSound(SND_CHANNEL_WEAPON);
		stateTimer = gameLocal.time + 1000;
		ramTargetEnt = NULL;
		ramPosition = vec3_zero;

		Event_SetFlyBobStrength(spawnArgs.GetFloat("fly_bob_strength"));
		Event_SetTurnRate(spawnArgs.GetFloat("turn_rate"));

		//hide scan beams.
		for (int i = 0; i < SCANBEAMS; i++)
		{
			beamOrigin[i]->SetColor(FRIENDLYCOLOR);
			beamOrigin[i]->UpdateVisuals();
			beamOrigin[i]->Hide();
		}

		//Turn off light.
		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
			headlightHandle = -1;
		}

		//hide the laser.
		targetinglineStart->Hide();
	}
	else
	{

		for (int i = 0; i < SCANBEAMS; i++)
		{
			beamOrigin[i]->SetColor(FRIENDLYCOLOR);
			beamOrigin[i]->UpdateVisuals();
		}

	}

	scanchargeParticles->SetColor(FRIENDLYCOLOR);
	scanchargeParticles->UpdateVisuals();
}
