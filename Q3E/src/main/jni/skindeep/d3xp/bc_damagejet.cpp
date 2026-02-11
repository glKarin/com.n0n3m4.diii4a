#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "idlib/LangDict.h"

#include "bc_trigger_deodorant.h"
#include "bc_trigger_sneeze.h"

#include "ai/AI.h"
#include "bc_damagejet.h"

//TODO: make the particle effect last forever, and allow this ent to togggle it off. So that we don't need to make bespoke particle effect that lasts a specific time. Func emitter

const int JET_LERPTIME = 1500; //make the jet face toward enemy within this time duration.

CLASS_DECLARATION(idEntity, idDamageJet)
	EVENT(EV_SetAngles, idDamageJet::Event_SetAngles)
END_CLASS

void idDamageJet::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( lifetimer ); //  int lifetimer
	savefile->WriteInt( lifetimeMax ); //  int lifetimeMax
	savefile->WriteBool( noLifetime ); //  bool noLifetime

	savefile->WriteInt( damageTimer ); //  int damageTimer
	savefile->WriteInt( damageTimerMax ); //  int damageTimerMax

	savefile->WriteFloat( range ); //  float range
	savefile->WriteFloat( enemyRange ); //  float enemyRange

	savefile->WriteString( damageDefname ); // idStr damageDefname

	savefile->WriteVec3( direction ); //  idVec3 direction
	savefile->WriteBool( allowLerpRotate ); //  bool allowLerpRotate

	savefile->WriteInt( ownerIndex ); //  int ownerIndex

	savefile->WriteObject( emitterParticles ); //  idFuncEmitter * emitterParticles

	savefile->WriteRenderLight( headlight ); //  renderLight_t headlight
	savefile->WriteInt( headlightHandle ); //  int headlightHandle

	savefile->WriteInt( jetLerpstate ); //  int jetLerpstate
	savefile->WriteInt( jetLerpTimer ); //  int jetLerpTimer
	savefile->WriteVec3( jetInitialTarget ); //  idVec3 jetInitialTarget
	savefile->WriteVec3( jetDestinationTarget ); //  idVec3 jetDestinationTarget

	savefile->WriteAngles( jetCurrentAngle ); //  idAngles jetCurrentAngle


	savefile->WriteBool( hasCloud ); //  bool hasCloud
	savefile->WriteBool( hasSpark ); //  bool hasSpark
	savefile->WriteEntityDef( cloudDef ); // const  idDeclEntityDef * cloudDef
	savefile->WriteInt( cloudTimer ); //  int cloudTimer
}

void idDamageJet::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( lifetimer ); //  int lifetimer
	savefile->ReadInt( lifetimeMax ); //  int lifetimeMax
	savefile->ReadBool( noLifetime ); //  bool noLifetime

	savefile->ReadInt( damageTimer ); //  int damageTimer
	savefile->ReadInt( damageTimerMax ); //  int damageTimerMax

	savefile->ReadFloat( range ); //  float range
	savefile->ReadFloat( enemyRange ); //  float enemyRange

	savefile->ReadString( damageDefname ); // idStr damageDefname

	savefile->ReadVec3( direction ); //  idVec3 direction
	savefile->ReadBool( allowLerpRotate ); //  bool allowLerpRotate

	savefile->ReadInt( ownerIndex ); //  int ownerIndex

	savefile->ReadObject( CastClassPtrRef(emitterParticles) ); //  idFuncEmitter * emitterParticles

	savefile->ReadRenderLight( headlight ); //  renderLight_t headlight
	savefile->ReadInt( headlightHandle ); //  int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( jetLerpstate ); //  int jetLerpstate
	savefile->ReadInt( jetLerpTimer ); //  int jetLerpTimer
	savefile->ReadVec3( jetInitialTarget ); //  idVec3 jetInitialTarget
	savefile->ReadVec3( jetDestinationTarget ); //  idVec3 jetDestinationTarget

	savefile->ReadAngles( jetCurrentAngle ); //  idAngles jetCurrentAngle


	savefile->ReadBool( hasCloud ); //  bool hasCloud
	savefile->ReadBool( hasSpark ); //  bool hasSpark
	savefile->ReadEntityDef( cloudDef ); // const  idDeclEntityDef * cloudDef
	savefile->ReadInt( cloudTimer ); //  int cloudTimer
}


idDamageJet::idDamageJet(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idDamageJet::~idDamageJet(void)
{
	if (emitterParticles)
	{
		delete emitterParticles;
	}

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}


void idDamageJet::Spawn(void)
{
    //intervals between damage bursts.
    damageTimerMax = (int)(spawnArgs.GetFloat("delay", ".3") * 1000.0f);
    damageTimer = gameLocal.time + damageTimerMax;

	enemyRange = spawnArgs.GetFloat("enemyrange", "128"); //We cheat the distance for enemies; we extend a bit further to damage them.
    range = spawnArgs.GetFloat("range", "96");

    lifetimeMax = (int)(spawnArgs.GetFloat("lifetime") * 1000.0f);
	noLifetime = spawnArgs.GetBool("no_lifetime", "0");
	allowLerpRotate = spawnArgs.GetBool("allow_lerp_rotate", "1");
    damageDefname = spawnArgs.GetString("def_damage", "damage_generic");


	//We want to avoid having the jet shoot into geometry (nearby wall, nearby ceiling, nearby ground, etc)

	jetCurrentAngle = FindSafeInitialAngle();



    //Spawn the particle effect.
	idAngles sprayAngle = jetCurrentAngle;
    sprayAngle.pitch += 90;

	//const char *zz = spawnArgs.GetString("model_jet");
	idDict args;
	args.Clear();
	args.Set("model",  spawnArgs.GetString("model_jet"));	
	args.Set("start_off", "1");
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetMatrix("rotation", sprayAngle.ToMat3());
	emitterParticles = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	if (!emitterParticles)
	{
		gameLocal.Warning("DamageJet '%s' unable to spawn particle emitter '%s'\n", GetName(), spawnArgs.GetString("model_jet", ""));
	}
	else
	{
		emitterParticles->SetActive(true);
	}
    
	StartSound("snd_jet", SND_CHANNEL_BODY);

    direction = jetCurrentAngle.ToForward();

    ownerIndex = spawnArgs.GetInt("ownerindex");    

    lifetimer = gameLocal.time + lifetimeMax;


	idVec3 lightColor = spawnArgs.GetVector("lightcolor");
	headlight.shader = declManager->FindMaterial(spawnArgs.GetString("mtr_light"), false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 64.0f;
	headlight.shaderParms[0] = lightColor.x;
	headlight.shaderParms[1] = lightColor.y;
	headlight.shaderParms[2] = lightColor.z;
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);

	headlight.origin = GetPhysics()->GetOrigin() + direction * 4.0f; //move it slightly away from the origin point, so that it doesn't clip into the pipe.
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);


	jetLerpstate = JET_DORMANT;
	jetLerpTimer = 0;
	jetInitialTarget = GetPhysics()->GetOrigin() + direction * 64;
	jetDestinationTarget = vec3_zero;

    BecomeActive(TH_THINK);

	
	idStr cloudName = spawnArgs.GetString("def_cloud");
	hasCloud = (cloudName[0] != '\0');
	if (hasCloud)
	{
		cloudDef = gameLocal.FindEntityDef(cloudName, false);
		if (!cloudDef)
		{
			common->Error("'%s' failed to load def_cloud '%s'\n", GetName(), cloudName.c_str());
		}
	}

	hasSpark = spawnArgs.GetBool("emitsSpark");
	
	cloudTimer = gameLocal.time + 100;
}

idAngles idDamageJet::FindSafeInitialAngle()
{
	

	//original angle.
	idVec3 dir = spawnArgs.GetVector("dir");
	dir.Normalize();

	float range = spawnArgs.GetFloat("range", "96");

	//Check if it intersects with geometry.
	trace_t tr;
	gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + dir * range, MASK_SOLID, NULL);

	if (tr.fraction >= 1)
	{
		//not hitting any geometry. So just return the original value.
		return spawnArgs.GetVector("dir").ToAngles();
	}
	
	const int ARRAYSIZE = 6;
	idVec3 angleArray[] =
	{
		idVec3(0, 1, 0),
		idVec3(0, -1, 0),
		idVec3(1, 0, 0),
		idVec3(-1, 0, 0),
		idVec3(0, 0, 1),
		idVec3(0, 0, -1),
	};

	//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + dir * 128, 4, 900000);

	//Try to find which cardinal direction we're closest to.
	float bestResult = -1;
	int bestIndex = -1;
	for (int i = 0; i < ARRAYSIZE; i++)
	{
		float facingResult = DotProduct(dir, angleArray[i]);

		//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + angleArray[i] * 64, 4, 9000000);
		//gameRenderWorld->DrawTextA(idStr::Format("%.2f", facingResult), GetPhysics()->GetOrigin() + angleArray[i] * 64, .2f, colorGreen, gameLocal.GetLocalPlayer()->viewAxis, 1, 900000);

		if (facingResult > bestResult)
		{
			bestResult = facingResult;
			bestIndex = i;
		}
	}

	if (bestIndex < 0)
	{
		//Something went weird. Just exit here.
		return spawnArgs.GetVector("dir").ToAngles();
	}


	trace_t tr2;
	idVec3 bestCandidateAngle = angleArray[bestIndex];
	gameLocal.clip.TracePoint(tr2, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + bestCandidateAngle * range, MASK_SOLID, NULL);

	if (tr2.fraction >= 1 || tr2.fraction > tr.fraction)
	{
		//The candidate is valid, OR, is at least better than the original direction (the traceline reached further). Return the candidate.
		return bestCandidateAngle.ToAngles();
	}

	//Give up, just return the original 
	return spawnArgs.GetVector("dir").ToAngles();
}

void idDamageJet::Think(void)
{
	if (IsHidden())
		return;

    if (gameLocal.time > damageTimer)
    {
        damageTimer = gameLocal.time + damageTimerMax;

        int entityCount;
        idEntity *entityList[MAX_GENTITIES];

		
        entityCount = gameLocal.EntitiesWithinRadius(this->GetPhysics()->GetOrigin(), max(enemyRange, range), entityList, MAX_GENTITIES);

        for (int i = 0; i < entityCount; i++)
        {
            idEntity *ent = entityList[i];

            if (!ent)
                continue;

            if (ent->IsHidden() || !ent->fl.takedamage || ent->entityNumber == ownerIndex)
                continue;

            idVec3 entityCenterMass = ent->GetPhysics()->GetAbsBounds().GetCenter();
            idVec3 dirToEntity = entityCenterMass - this->GetPhysics()->GetOrigin();
            dirToEntity.Normalize();
            float facingResult = DotProduct(dirToEntity, direction);

            if (facingResult > 0)
            {
                //Ok, found an entity that's within a 180 degree cone of the damage jet.

				//do the distance check.
				if (!ent->IsType(idAI::Type))
				{
					//not an AI. AI and non-AI have different distance checks, so do a distance check here.
					float distance = (this->GetPhysics()->GetOrigin() - entityCenterMass).LengthFast();
					
					if (distance >= range) //it's too far. Skip it.
						continue;
				}

				//Inflict damage.
				const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefname);
				if (damageDef)
				{
					bool doDamage = false;
					if (damageDef->GetBool("isfire"))
					{
						if (gameLocal.time > ent->lastFireattachmentDamagetime)
						{
							ent->lastFireattachmentDamagetime = gameLocal.time + 300;
							doDamage = true;
						}
					}
					else
					{
						doDamage = true;
					}

					if (doDamage)
					{
						ent->Damage(this, this, vec3_zero, damageDefname, 1.0f, 0, 0);
					}

					if (jetLerpstate == JET_DORMANT && allowLerpRotate)
					{
						jetLerpstate = JET_LERPING;
						jetLerpTimer = gameLocal.time;

						//we 'cheat' a little and make the visual jet point toward the first enemy it's affecting. This is only a cosmetic change, no logic is affected by this.
						idVec3 adjustedAngle = (ent->GetPhysics()->GetAbsBounds().GetCenter() - this->GetPhysics()->GetOrigin());
						adjustedAngle.Normalize();
						jetDestinationTarget = GetPhysics()->GetOrigin() + (adjustedAngle * 64);
					}
				}
            }
        }
    }

	if (gameLocal.time > cloudTimer && (hasCloud || hasSpark))
	{
		cloudTimer = gameLocal.time + 300; //how often to spew a cloud.

		//Find the endpoint of the jet.
		idVec3 cloudPos = GetPhysics()->GetOrigin();
		idAngles emitterAngle = jetCurrentAngle;
		cloudPos += emitterAngle.ToForward() * range;

		if (hasCloud)
		{
			SpawnCloud(cloudDef, cloudPos);
		}

		if (hasSpark)
		{
			gameLocal.CreateSparkObject(cloudPos, emitterAngle.ToForward());
		}
	}

	if (jetLerpstate == JET_LERPING)
	{
		float lerp = (gameLocal.time - jetLerpTimer) / (float)JET_LERPTIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = idMath::CubicEaseInOut(lerp);
		
		idVec3 adjustedTargetPos;
		adjustedTargetPos.x = idMath::Lerp(jetInitialTarget.x, jetDestinationTarget.x, lerp);
		adjustedTargetPos.y = idMath::Lerp(jetInitialTarget.y, jetDestinationTarget.y, lerp);
		adjustedTargetPos.z = idMath::Lerp(jetInitialTarget.z, jetDestinationTarget.z, lerp);
		
		idVec3 newVecDir = adjustedTargetPos - GetPhysics()->GetOrigin();
		newVecDir.Normalize();
		
		idAngles newAngle = newVecDir.ToAngles();

		jetCurrentAngle = newAngle;

		//We need to do some hack stuff to make particles work right, since their forward angle always need to add +90 pitch
		newAngle.pitch += 90;
		emitterParticles->SetAngles(newAngle);

		if (lerp >= 1)
			jetLerpstate = JET_LERPDONE;
	}

    if (gameLocal.time >= lifetimer && !noLifetime)
    {
		if (emitterParticles)
		{
			emitterParticles->SetActive(false); //Stop particles.
		}

		//kill light.
		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
			headlightHandle = -1;
		}

		StopSound(SND_CHANNEL_ANY, false); //Stop sound.

        BecomeInactive(TH_THINK);
        this->Hide();
        PostEventMS(&EV_Remove, 1000); //add a little delay so the particle has time to stop.
    }
}

void idDamageJet::SpawnCloud(const idDeclEntityDef	*cloudEntityDef, idVec3 pos)
{
	int spewType = cloudEntityDef->dict.GetInt("spewtype");
	int radius = cloudEntityDef->dict.GetInt("spewRadius", "64");

	idDict args;
	args.Clear();
	args.SetVector("origin", pos);
	args.SetVector("mins", idVec3(-radius, -radius, -radius));
	args.SetVector("maxs", idVec3(radius, radius, radius));
	args.SetBool("highlight_boundcheck", false);

	idTrigger_Multi* spewTrigger;
	args.Set("spewParticle", cloudEntityDef->dict.GetString("spewParticle", "pepperburst01.prt"));
	args.SetInt("spewLifetime", cloudEntityDef->dict.GetInt("spewLifetime", "9000"));
	if (spewType == 0)
	{
		args.SetFloat("multiplier", cloudEntityDef->dict.GetFloat("sneezemultiplier", "1"));
		args.Set("classname", "trigger_cloud_sneeze");
		spewTrigger = (idTrigger_sneeze*)gameLocal.SpawnEntityDef(args);

	}
	else if (spewType == 1)
	{
		args.Set("classname", "trigger_cloud_deodorant");
		spewTrigger = (idTrigger_deodorant*)gameLocal.SpawnEntityDef(args);
	}
	else
	{
		gameLocal.Error("Missing spew spawn logic in '%s'\n", this->GetName());
	}


	if (ownerIndex >= 0 && ownerIndex < MAX_GENTITIES - 2)
	{
		idEntity *ownerEnt = gameLocal.entities[ownerIndex];
		if (ownerEnt != NULL)
		{
			gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_cloud_created"), ownerEnt->displayName.c_str()), GetPhysics()->GetOrigin(), true, 0, false); // SW 6th May 2025: anti-spam measures
		}
	}
}

void idDamageJet::Event_SetAngles(idAngles const& angles)
{
	jetCurrentAngle = angles;
	direction = angles.ToForward();
	idAngles particleAng = angles;
	particleAng.pitch += 90;
	emitterParticles->SetAngles(particleAng);
}