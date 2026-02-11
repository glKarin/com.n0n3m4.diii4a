#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "SmokeParticles.h"
#include "framework/DeclEntityDef.h"
#include "Trigger.h"
#include "bc_glasspiece.h"

#include "bc_chembomb.h"



const int DETONATION_OFFSET = 1;
const int SPOKECOUNT = 5;
const int DECALSIZE = 96;
const int PAINTRIGGERRADIUS = 32;
const int PAINTRIGGERHEIGHTRADIUS = 8;
const int PUDDLE_LIFETIME = 10000;
const int ACTOR_SPLASHRADIUS = 32;
const int DECALSIZE_BODY = 16;

CLASS_DECLARATION(idMoveableItem, idChembomb)

END_CLASS

//TODO: make some sort of cue when puddle nears its lifetime.

void idChembomb::Spawn(void)
{
	this->fl.takedamage = true;

	smokeFlyTime = 0;

	const char *smokeName = spawnArgs.GetString("smoke_fly");
	if (*smokeName != '\0')
	{
		smokeFly = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, smokeName));
		smokeFlyTime = gameLocal.time;
	}

}

void idChembomb::Save(idSaveGame *savefile) const
{
	savefile->WriteParticle( smokeFly ); // const  idDeclParticle *	 smokeFly
	savefile->WriteInt( smokeFlyTime ); //  int smokeFlyTime
}

void idChembomb::Restore(idRestoreGame *savefile)
{
	savefile->ReadParticle( smokeFly ); // const  idDeclParticle *	 smokeFly
	savefile->ReadInt( smokeFlyTime ); //  int smokeFlyTime
}

bool idChembomb::Collide(const trace_t &collision, const idVec3 &velocity)
{
	idAngles sprayAngle;
	sprayAngle = (collision.c.normal).ToAngles();
	sprayAngle.pitch += 90;

	idMoveableItem::Collide(collision, velocity);


	//v = -(velocity * collision.c.normal);
	//
	//if (v < 1)
	//	return false;

	//gameRenderWorld->DrawTextA(idStr::Format("%f", v), GetPhysics()->GetOrigin(), .5f, colorWhite, collision.c.normal.ToMat3(), 1, 3000);

	//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + (collision.c.normal * DETONATION_OFFSET), 1, 10000);

	Detonate(GetPhysics()->GetOrigin() + (collision.c.normal * DETONATION_OFFSET), sprayAngle.ToMat3());
	return false;
}

void idChembomb::Think(void)
{
	if (this->IsHidden())
		return;

	if (smokeFly != NULL && smokeFlyTime && !IsHidden())
	{
		idVec3 dir = -GetPhysics()->GetLinearVelocity();
		dir.Normalize();

		//SetTimeState ts(originalTimeGroup);		

		if (!gameLocal.smokeParticles->EmitSmoke(smokeFly, smokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup /*_D3XP*/))
		{
			smokeFlyTime = gameLocal.time;
		}
	}

	RunPhysics();
	Present();
}



void idChembomb::Detonate(idVec3 detonationPos, idMat3 particleDirection)
{
	int spokeDelta;
	int initialRandomYaw;
	int wallSplashCount = 0; //For wall splashes, try to prevent too many of them from being grouped up, since they'd just overlap one another.
	trace_t downTr;
	idDebris *debris;
	idEntity *debrisEnt;
	const idDeclEntityDef *debrisDef;
	idAngles flyDirection = particleDirection.ToAngles();
	flyDirection.pitch -= 90;

	if (this->IsHidden())
		return;

	
	gameLocal.SetSuspiciousNoise(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetInt("noise_radius", "300"), NOISE_LOWPRIORITY);

	this->Hide();
	fl.takedamage = false;

	idEntity *glassEnt;
	const idDeclEntityDef *glassDef;

	debrisDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_grenadespoon"), false);
	gameLocal.SpawnEntityDef(debrisDef->dict, &debrisEnt, false);
	if (debrisEnt)
	{	
		if (debrisEnt->IsType(idDebris::Type))
		{
			debris = static_cast<idDebris *>(debrisEnt);
			debris->Create(NULL, detonationPos, mat3_identity);
			debris->SetOrigin(detonationPos);
			debris->Launch();
			debris->GetPhysics()->SetLinearVelocity((flyDirection.ToForward() * 64) + idVec3(0, 0, 128));
			debris->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.CRandomFloat() * 128, gameLocal.random.CRandomFloat() * 128, gameLocal.random.CRandomFloat() * 128));
		}
	}

	//debris_glass
	//moveable_chair
	glassDef = gameLocal.FindEntityDef("debris_glass", false);
	if (gameLocal.SpawnEntityDef(glassDef->dict, &glassEnt, false))
	{
		glassEnt->SetOrigin(detonationPos);
		if (glassEnt && glassEnt->IsType(idGlassPiece::Type))
		{
			idGlassPiece *glassDebris = static_cast<idGlassPiece *>(glassEnt);
			glassDebris->Create(detonationPos, mat3_identity);
			glassDebris->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.CRandomFloat() * 64, gameLocal.random.CRandomFloat() * 64, gameLocal.random.CRandomFloat() * 64));
			glassDebris->GetPhysics()->SetLinearVelocity((flyDirection.ToForward() * 32) + idVec3(0, 0, 48));
			//glassDebris->GetPhysics()->SetGravity(vec3_zero);


			//gameRenderWorld->DebugSphere(colorGreen, idSphere(glassDebris->GetPhysics()->GetOrigin(), 1), 5000);
		}
	}


	//gameRenderWorld->DebugArrow(colorCyan, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0, 0, -128), 4, 5000);

	
	
	StartSound("snd_shatter", SND_CHANNEL_ANY, 0, false, NULL);
	idEntityFx::StartFx("fx/chembomb_explode", &detonationPos, &particleDirection, NULL, false);
	
	ChemRadiusAttack(detonationPos);
	
	initialRandomYaw = gameLocal.random.RandomInt(90);	
	
	spokeDelta = 360 / SPOKECOUNT;
	for (int i = 0; i < SPOKECOUNT; i++)
	{
		trace_t tr1;
		idAngles spokeAng = idAngles(0, initialRandomYaw + (spokeDelta * i), 0);
	
		gameLocal.clip.TracePoint(tr1, detonationPos, detonationPos + (spokeAng.ToForward() * 64), MASK_SOLID, this);
	
		if (tr1.fraction < 1)
		{
			//gameRenderWorld->DebugSphere(colorRed, idSphere(tr1.endpos, 2), 3000);
			if (tr1.fraction <= .3f)
			{
				if (wallSplashCount <= 0)
				{
					wallSplashCount++;
					SpawnChemPuddle(tr1, true);
				}
			}
			else
			{
				SpawnChemPuddle(tr1, false);
			}
		}
		else
		{
			trace_t tr2;
			//gameRenderWorld->DebugArrow(colorRed, detonationPos, detonationPos + (spokeAng.ToForward() * 64), 4, 3000);
	
			gameLocal.clip.TracePoint(tr2, tr1.endpos, tr1.endpos + idVec3(0,0,-4096), MASK_SOLID, this);
	
			if (tr2.fraction < 1)
			{
				//gameRenderWorld->DebugSphere(colorRed, idSphere(tr2.endpos, 2), 3000);
				SpawnChemPuddle(tr2, false);
			}
		}
	}
	
	//Do a puddle straight below grenade.
	gameLocal.clip.TracePoint(downTr, detonationPos, detonationPos + idVec3(0,0,-4096), MASK_SOLID, this);
	SpawnChemPuddle(downTr, true);

	//gameRenderWorld->DebugArrow(colorYellow, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idVec3(0, 0, 128), 4, 5000);
	PostEventMS(&EV_Remove, 100);
}

void idChembomb::ChemRadiusAttack(idVec3 epicenter)
{
	int i;
	int entityCount;
	idEntity *entityList[MAX_GENTITIES];

	entityCount = gameLocal.EntitiesWithinRadius(epicenter, ACTOR_SPLASHRADIUS, entityList, MAX_GENTITIES);

	for (i = 0; i < entityCount; i++)
	{
		idEntity *ent = entityList[i];
		trace_t splashTr;

		if (!ent)
			continue;

		if (!ent->IsType(idActor::Type) || ent->IsHidden())
			continue;

		//we now have an actor.
		gameLocal.clip.TracePoint(splashTr, epicenter, ent->GetPhysics()->GetAbsBounds().GetCenter(), MASK_SHOT_RENDERMODEL, this);

		if (splashTr.fraction < 1)
		{
			//FIX THIS
			idVec3 trDir = splashTr.endpos - epicenter;
			trDir.NormalizeFast();

			gameRenderWorld->DebugArrow(colorGreen, epicenter, epicenter + (trDir * 8) , 1, 5000);
			ent->ProjectOverlay(epicenter,  trDir, DECALSIZE_BODY, "textures/decals/chempuddle_body");
			//textures/decals/chempuddle
			//gameLocal.ProjectDecal(splashTr.endpos, -splashTr.c.normal, 8.0f, true, DECALSIZE, "textures/decals/chempuddle");
		}
	}
}

void idChembomb::SpawnChemPuddle(trace_t traceInfo, bool playAudio)
{
	idDict args;
	idAngles sprayAngle;
	sprayAngle = (traceInfo.c.normal).ToAngles();
	sprayAngle.pitch += 90;

	//Burn mark.
	gameLocal.ProjectDecal(traceInfo.endpos, -traceInfo.c.normal, 8.0f, true, DECALSIZE, "textures/decals/chempuddle_ashes");	

	//Decal.
	gameLocal.ProjectDecal(traceInfo.endpos, -traceInfo.c.normal, 8.0f, true, DECALSIZE, "textures/decals/chempuddle");

	//FX.
	idEntityFx::StartFx( "fx/chembomb_sizzle", traceInfo.endpos, sprayAngle.ToMat3());

	//Spawn pain trigger.
	args.Clear();
	args.SetVector("origin", traceInfo.endpos + traceInfo.c.normal * PAINTRIGGERHEIGHTRADIUS);
	args.SetVector("mins", idVec3(-PAINTRIGGERRADIUS, -PAINTRIGGERRADIUS, -PAINTRIGGERHEIGHTRADIUS));
	args.SetVector("maxs", idVec3(PAINTRIGGERRADIUS, PAINTRIGGERRADIUS, PAINTRIGGERHEIGHTRADIUS));
	args.SetFloat("delay", .3f);
	args.Set("def_damage", spawnArgs.GetString("def_puddledamage", "damage_chem"));
	args.Set("snd_sizzle", "chem_sizzle");
	
	idTrigger_Hurt* hurtTrigger = (idTrigger_Hurt *)gameLocal.SpawnEntityType(idTrigger_Hurt::Type, &args);
	hurtTrigger->SetAxis(sprayAngle.ToMat3());
	hurtTrigger->StartSound("snd_sizzle", SND_CHANNEL_BODY, 0, false, NULL);
	hurtTrigger->PostEventMS(&EV_Remove, PUDDLE_LIFETIME);
	hurtTrigger->PostEventMS(&EV_StopSound, PUDDLE_LIFETIME - 100, SND_CHANNEL_BODY, 0);

	hurtTrigger->PostEventMS(&EV_StartSoundShader, PUDDLE_LIFETIME - 300, "chem_fadeout", SND_CHANNEL_BODY2);
}

void idChembomb::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	int damage;
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(damageDefName, false);

	if (!fl.takedamage)
		return;

	if (!damageDef)
	{
		common->Warning("%s unable to find damagedef %s\n", this->GetName(), damageDefName);
		return;
	}

	//Take damage.
	damage = damageDef->dict.GetInt("damage", "0");

	if (damage > 0)
	{
		//Inflict damage.
		health -= damage;
	}

	if (health <= 0)
	{
		Detonate(GetPhysics()->GetOrigin(), mat3_identity);
	}
}



