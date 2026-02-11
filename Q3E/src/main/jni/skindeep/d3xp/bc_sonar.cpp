#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "framework/DeclEntityDef.h"

//#include "ai/AI.h"
#include "bc_cat.h"
#include "bc_skullsaver.h"
#include "bc_sonar.h"


//TODO: turn off player collision on sonar

const int PINGTIME_MAX = 3000;
const int SONAR_RADIUS = 1024;


const float GLITCH_OFFSET_MIN = 4.0f;
const float GLITCH_OFFSET_MAX = 64.0f;

const int GLITCH_TIME_MIN = 1200;
const int GLITCH_TIME_MAX = 10;

CLASS_DECLARATION(idAnimated, idSonar)

END_CLASS


void idSonar::Spawn(void)
{
	idDict args;
	jointHandle_t radarJoint;
	idVec3 radarPos;
	idMat3 radarMat3;
	//idAngles spawnAngle = spawnArgs.GetAngles("angles");
	//this->SetAngles(spawnAngle);
	

	//Spawn and bind particle to radar.
	args.Clear();
	args.Set("model", "sonar_idle.prt");
	args.Set("start_off", "1");
	particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	radarJoint = this->GetAnimator()->GetJointHandle("prong");
	this->GetJointWorldTransform(radarJoint, gameLocal.time, radarPos, radarMat3);
	particleEmitter->SetOrigin(radarPos);	
	particleEmitter->BindToJoint(this, radarJoint, true);	

	timer = this->Event_PlayAnim("opening", 1);

	//StartSound("snd_ticking", SND_CHANNEL_BODY, 0, false, NULL);

	//Dust kickup when affixed to surface.
	idEntityFx::StartFx("fx/smoke_ring04", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis());

	maxHealth = spawnArgs.GetInt("health");

	state = SONAR_OPENING;
	sonarPingTime = 0;
	hasPlayedPingSound = false;
	glitchTimer = 0;

	StartSound("snd_deploy", SND_CHANNEL_ANY, 0, false, NULL);

	damageParticles = false;
}

void idSonar::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( maxHealth ); // int maxHealth

	savefile->WriteInt( timer ); // int timer

	savefile->WriteObject( particleEmitter ); // idFuncEmitter * particleEmitter

	savefile->WriteInt( sonarPingTime ); // int sonarPingTime

	savefile->WriteBool( hasPlayedPingSound ); // bool hasPlayedPingSound

	SaveFileWriteArray( offsets, 256, WriteInt ); // int offsets[256]

	savefile->WriteInt( offsetIndex ); // int offsetIndex
	savefile->WriteInt( glitchTimer ); // int glitchTimer

	savefile->WriteBool( damageParticles ); // bool damageParticles
}

void idSonar::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( maxHealth ); // int maxHealth

	savefile->ReadInt( timer ); // int timer

	savefile->ReadObject( CastClassPtrRef(particleEmitter) ); // idFuncEmitter * particleEmitter

	savefile->ReadInt( sonarPingTime ); // int sonarPingTime

	savefile->ReadBool( hasPlayedPingSound ); // bool hasPlayedPingSound

	SaveFileReadArray( offsets, ReadInt ); // int offsets[256]

	savefile->ReadInt( offsetIndex ); // int offsetIndex
	savefile->ReadInt( glitchTimer ); // int glitchTimer

	savefile->ReadBool( damageParticles ); // bool damageParticles
}

void idSonar::Think(void)
{
	if (state == SONAR_OPENING)
	{
		if (gameLocal.time >= timer)
		{
			state = SONAR_IDLE;
			this->Event_PlayAnim("idle", 1, true);
			
			isFrobbable = true;
			particleEmitter->PostEventMS(&EV_Activate, 0, this);
		}
	}
	else if (state == SONAR_IDLE)
	{
		int i;
		int pingEntities = 0;

		if (gameLocal.time > sonarPingTime)
		{			
			sonarPingTime = gameLocal.time + PINGTIME_MAX;
			hasPlayedPingSound = false;
			
			idEntityFx::StartFx("fx/sonar_ping", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		}

		if (health < maxHealth)
		{
			if (gameLocal.time > glitchTimer)
			{
				float lerpHealth = health / (float)maxHealth;

				for (i = 0; i < 256; i++)
				{
					offsets[i] = gameLocal.random.CRandomFloat() * idMath::Lerp( GLITCH_OFFSET_MAX, GLITCH_OFFSET_MIN, lerpHealth);
				}

				glitchTimer = gameLocal.time + idMath::Lerp(GLITCH_TIME_MAX, GLITCH_TIME_MIN, lerpHealth);
			}
		}

		offsetIndex = 0;

		for (idEntity* ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
		{
			idVec3 dirToTarget;

			if (!ent)
				continue;

			if (ent == NULL)
				continue;

			if (ent->health <= 0 || ent->IsHidden() || ent == this || ent->entityNumber < 0 || ent->entityNumber >= MAX_GENTITIES - 2 || ent->fl.isDormant)
				continue;			

			//Distance check.
			float distanceToEnt = (ent->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
			if (distanceToEnt > SONAR_RADIUS)
				continue;			

			if (ent->IsType(idActor::Type) && ent->team == TEAM_ENEMY)
			{
				/* //Draw arrow pointing to entities.
				dirToTarget = ent->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
				dirToTarget.NormalizeFast();

				gameRenderWorld->DebugArrow(colorSonar, this->GetPhysics()->GetOrigin() + (dirToTarget * 12),
					this->GetPhysics()->GetOrigin() + (dirToTarget * 24), 1, 10);
					*/

				if (ent != gameLocal.GetLocalPlayer()) //Don't draw player hands.
				{
					DrawMarkerAtJoint("lhand", ent);
					DrawMarkerAtJoint("rhand", ent);
				}

				DrawMarkerAtJoint("lfoot", ent);
				DrawMarkerAtJoint("rfoot", ent);

				DrawMarkerAtJoint("head", ent);
				DrawMarkerAtJoint(ent->spawnArgs.GetString("bone_sonarheart", "shoulders"), ent, 1);
			}
			else if (ent->IsType(idCat::Type))
			{
				DrawMarkerAtJoint("foot_front_l", ent);
				DrawMarkerAtJoint("foot_front_r", ent);

				DrawMarkerAtJoint("foot_rear_l", ent);
				DrawMarkerAtJoint("foot_rear_r", ent);

				DrawMarkerAtJoint("head", ent);
				DrawMarkerAtJoint("body", ent, 1);
			}
			else if (ent->IsType(idSkullsaver::Type))
			{
				//Draw marker on skullsaver.
				gameRenderWorld->DebugCircle(colorSonar, ent->GetPhysics()->GetOrigin(), gameLocal.GetLocalPlayer()->viewAngles.ToForward(), 4, 4, 10);
			}
			else
			{
				continue;
			}

			pingEntities++;
		}

		if (!hasPlayedPingSound)
		{
			hasPlayedPingSound = true;

			if (health >= maxHealth)
			{
				switch (pingEntities)
				{
				case 1: StartSound("snd_ping1", SND_CHANNEL_ANY, 0, false, NULL); break;
				case 2: StartSound("snd_ping2", SND_CHANNEL_ANY, 0, false, NULL); break;
				case 3: StartSound("snd_ping3", SND_CHANNEL_ANY, 0, false, NULL); break;
				case 4: StartSound("snd_ping4", SND_CHANNEL_ANY, 0, false, NULL); break;
				case 5: StartSound("snd_ping5", SND_CHANNEL_ANY, 0, false, NULL); break;
				default: StartSound("snd_ping6_plus", SND_CHANNEL_ANY, 0, false, NULL); break;
				}
			}
			else
			{
				//Sonar is damaged. Play a glitchy sound.
				StartSound("snd_glitch", SND_CHANNEL_ANY, 0, false, NULL); 
				
			}
		}
	}
	else if (state == SONAR_CLOSING)
	{
		if (gameLocal.time > timer)
		{
			//Give to player.
			state = SONAR_PACKUPDONE;

			this->Hide();
			PostEventMS(&EV_Remove, 100);
			idEntityFx::StartFx("fx/smoke_ring04", &GetPhysics()->GetOrigin(), &mat3_default, NULL, false);

			//Give sonar back to player.
			//gameLocal.GetLocalPlayer()->Give("weapon", "weapon_sonar");
			//gameLocal.GetLocalPlayer()->SetAmmoDelta("ammo_sonar", 1);
			gameLocal.GetLocalPlayer()->GiveItem("weapon_sonar");
		}
	}

	idAnimated::Think();
	idAnimated::Present();
}

void idSonar::DrawMarkerAtJoint(const char *jointName, idEntity *ent, int markerType)
{
	jointHandle_t joint;
	idVec3 jointPos;
	idMat3 jointMat3;
	float markerSize = 2;

	joint = ent->GetAnimator()->GetJointHandle(jointName);
	((idActor *)ent)->GetJointWorldTransform(joint, gameLocal.time, jointPos, jointMat3);

	if (markerType == 1)
	{
		//Link heartbeat to character health

		float healthLerp = ent->health / (float)ent->spawnArgs.GetInt("health");
		float heartSpeed = idMath::Lerp(.03f, .01f, healthLerp);

		markerSize = 3 + (idMath::Cos(gameLocal.time * heartSpeed) * 1.5f);
	}
	
	if (health < maxHealth)
	{
		jointPos.z += offsets[offsetIndex];
	}

	gameRenderWorld->DebugCircle(colorSonar, jointPos, gameLocal.GetLocalPlayer()->viewAngles.ToForward(), markerSize, 4, 10);

	if (health < maxHealth)
	{
		offsetIndex++;
	}
}




bool idSonar::DoFrob(int index, idEntity * frobber)
{

	if (state != SONAR_CLOSING)
	{
		state = SONAR_CLOSING;

		fl.takedamage = false;
		timer = this->Event_PlayAnim("closing", 1);

		gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);
	}

	return true;
}

void idSonar::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
	
	if (!damageDef)
	{
		gameLocal.Error("Unknown damageDef '%s'\n", damageDefName);
	}

	int	damage = damageDef->GetInt("damage");

	if (damage)
	{
		health -= damage;

		if (health <= 0)
		{
			//Sonar death.
			idVec3 myDir;

			state = SONAR_DESTROYED;
			fl.takedamage = false;
			this->Hide();

			idMoveableItem::DropItemsBurst(this, "gib", idVec3(8, 0, 0));

			PostEventMS(&EV_Remove, 100);

			//explosion fx.
			idEntityFx::StartFx("fx/explosion", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

			//Decal.
			myDir = GetPhysics()->GetAxis().ToAngles().ToForward();
			gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), -myDir, 8.0f, true, 40, "textures/decals/scorch");


			
			

			return;
		}
		else
		{
			//spark/damage fx.
			idAngles particleAngle;

			particleAngle = GetPhysics()->GetAxis().ToAngles();
			particleAngle.pitch += 90;

			
			idEntityFx::StartFx("fx/machine_sparkdamage", GetPhysics()->GetOrigin(), particleAngle.ToMat3());
			

			if (!damageParticles)
			{
				idDict args;
				idAngles particleAngle;
				idEntity *damageFX;

				particleAngle = GetPhysics()->GetAxis().ToAngles();
				particleAngle.pitch += 90;

				damageFX = idEntityFx::StartFx("fx/machine_damaged", GetPhysics()->GetOrigin(), particleAngle.ToMat3());
				damageFX->Bind(this, true);

				damageParticles = true;
			}
		}
	}

	
}

