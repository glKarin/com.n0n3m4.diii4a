#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "SmokeParticles.h"
#include "Game_local.h"
//#include "trigger.h"

#include "bc_ftl.h"
#include "bc_meta.h"
#include "bc_damagejet.h"
#include "bc_hazardpipe.h"

#define PARM_GLOW 7
const int FIREPIPE_AMOUNT_TO_ADD_TO_FTL_COUNTDOWN = 20000;

const int COOLDOWN_PRECOOLDOWNDELAY = 30; //accept damage for XX ms, so that things like shotguns can inflict multiple simultaneous damage hits.

//TODO: Sound effects stomp on each other, fix this

CLASS_DECLARATION(idStaticEntity, idHazardPipe)
END_CLASS

void idHazardPipe::Spawn(void)
{
	this->fl.takedamage = true;
	//fxActivate = spawnArgs.GetString("fx_activate");

	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	pipetype = spawnArgs.GetInt("hazardtype", "1"); //default to PIPETYPE_ELECTRICAL from bc_hazardpipe.h
	controlBox = NULL;
	pipeActive = true;
	sabotageEnabled = spawnArgs.GetBool("sabotage_enabled", "0");

	cooldownState = CD_IDLE;
	cooldownTimer = 0;

	idStr ambientSound = spawnArgs.GetString("snd_ambient");
	if (ambientSound.Length() > 0)
	{
		StartSound("snd_ambient", SND_CHANNEL_AMBIENT);
	}

	SetGlowLights(true);
}

void idHazardPipe::Event_PostSpawn(void)
{
}

void idHazardPipe::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( pipetype ); // int pipetype

	savefile->WriteObject( controlBox ); // idEntityPtr<idEntity> controlBox

	savefile->WriteBool( pipeActive ); // bool pipeActive

	savefile->WriteBool( sabotageEnabled ); // bool sabotageEnabled

	savefile->WriteInt( cooldownState ); // int cooldownState
	savefile->WriteInt( cooldownTimer ); // int cooldownTimer

}

void idHazardPipe::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( pipetype ); // int pipetype

	savefile->ReadObject( controlBox ); // idEntityPtr<idEntity> controlBox

	savefile->ReadBool( pipeActive ); // bool pipeActive


	savefile->ReadBool( sabotageEnabled ); // bool sabotageEnabled


	savefile->ReadInt( cooldownState ); // int cooldownState
	savefile->ReadInt( cooldownTimer ); // int cooldownTimer

}

void idHazardPipe::Think()
{
	idStaticEntity::Think();

	//Update the cooldown timer.
	if (cooldownState == CD_PRECOOLDOWN)
	{
		if (gameLocal.time >= cooldownTimer)
		{
			float cooldownTime = spawnArgs.GetFloat("cooldown", "0");
			cooldownState = CD_COOLINGDOWN;
			cooldownTimer = gameLocal.time + SEC2MS(cooldownTime);
		}
	}
	else if (cooldownState == CD_COOLINGDOWN)
	{
		if (gameLocal.time >= cooldownTimer)
		{
			//Exit cooldown state.
			cooldownState = CD_IDLE;
			SetGlowLights(true);
			BecomeInactive(TH_THINK);

			StartSound("snd_reactivate", SND_CHANNEL_BODY);
		}
	}
	
}

void idHazardPipe::SetGlowLights(bool value)
{
	renderEntity.shaderParms[PARM_GLOW] = value ? 1 : 0;
	UpdateVisuals();
}

void idHazardPipe::SetControlbox(idEntity *ent)
{
	controlBox = ent;
}

//Called when the electricalbox is repaired.
void idHazardPipe::ResetPipe()
{
	pipeActive = true;
	SetGlowLights(true);
}

void idHazardPipe::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	//Tell the control box to take damage.

	if (controlBox.IsValid())
	{
		controlBox.GetEntity()->Damage(this, this, vec3_zero, "damage_generic", 1.0f, 0, 0);
		if (controlBox.GetEntity()->health <= 0)
		{
			SetGlowLights(false);
		}
	}
}

void idHazardPipe::AddDamageEffect(const trace_t &collision, const idVec3 &velocity, const char *damageDefName)
{
	//Decal.
	gameLocal.ProjectDecal(collision.c.point, -collision.c.normal, 8.0f, true, 20.0f, "textures/decals/bullethole");

	if (controlBox.IsValid())
	{
		if (controlBox.GetEntity()->health <= 0)
		{
			//So we do this pipeActive bool here because Damage() is called BEFORE AddDamageEffect(), therefore the health gets set
			//to zero before the final damage effect can activate. So, this final bool ensures the final damage effect happens.
			if (!pipeActive)
			{
				return;
			}

			pipeActive = false;
		}
	}

	if (cooldownState == CD_COOLINGDOWN)
		return;

	//If this pipe can be 'sabotaged', add time to the FTL when it is damaged
	//if (sabotageEnabled)
	//{
	//	//Pipe has taken damage.
	//	idMeta *meta = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity());
	//	if (meta)
	//	{
	//		idFTL *ftl = static_cast<idFTL *>(meta->GetFTLDrive.GetEntity());
	//		if (ftl)
	//		{
	//			if (ftl->IsJumpActive(true))
	//			{
	//				if (ftl->AddToCountdown(FIREPIPE_AMOUNT_TO_ADD_TO_FTL_COUNTDOWN))
	//				{
	//					//time has successfully been added to ftl countdown.
	//					//make text appear.
	//
	//					idVec3 messagePos = collision.endpos;
	//
	//					//int seconds = MS2SEC(FIREPIPE_AMOUNT_TO_ADD_TO_FTL_COUNTDOWN);
	//					//const char *txt = idStr::Format("+%d SEC ADDED TO FTL COUNTDOWN", seconds);
	//					//gameLocal.GetLocalPlayer()->SetFlytextEvent(messagePos, txt, idDeviceContext::ALIGN_CENTER);
	//
	//					gameLocal.GetLocalPlayer()->SetFlytextEvent(messagePos, "+20 SEC ADDED TO FTL COUNTDOWN", idDeviceContext::ALIGN_CENTER);
	//				}
	//			}
	//		}
	//	}
	//}


	const idDeclEntityDef *jetDef;
	idDict args;
	idStr jetDefStr = spawnArgs.GetString("def_jet");

	jetDef = gameLocal.FindEntityDef(jetDefStr, false);
	if (!jetDef)
	{
		gameLocal.Error("hazardpipe '%s' cannot find def_jet '%s'", this->GetName(), spawnArgs.GetString("def_jet"));
		return;
	}
	
    args.Clear();
	args.Set("classname",		jetDefStr);
    args.SetVector("origin",	collision.c.point + collision.c.normal * 0.5f);
    args.SetFloat("delay",		jetDef->dict.GetFloat("delay", ".3"));
    args.SetFloat("range",		jetDef->dict.GetFloat("range", "96"));
    args.Set("def_damage",		jetDef->dict.GetString("def_damage"));
    args.SetFloat("lifetime",	jetDef->dict.GetFloat("lifetime", "4"));
    args.Set("model_jet",		jetDef->dict.GetString("model_jet"));
	args.Set("snd_jet",			jetDef->dict.GetString("snd_jet"));
	args.Set("mtr_light",		jetDef->dict.GetString("mtr_light"));
	args.SetVector("lightcolor",	jetDef->dict.GetVector("lightcolor"));
    args.SetVector("dir",		collision.c.normal);
    args.SetInt("ownerIndex",	this->entityNumber);
	args.Set("def_cloud",		jetDef->dict.GetString("def_cloud"));
    idDamageJet* damageJet = (idDamageJet *)gameLocal.SpawnEntityDef(args);    

	if (!damageJet)
	{
		gameLocal.Error("hazardpipe '%s' failed to spawn damagejet.");
	}


	//Note: sound is handled in the FX file. So that we can have multiple jets happening at same time.
	gameLocal.SetSuspiciousNoise(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetInt("noise_radius", "256"), NOISE_LOWPRIORITY);


	//Has a cooldown between damage instances. Shut down the pipe.
	float cooldownTime = spawnArgs.GetFloat("cooldown", "0");
	if (cooldownTime > 0 && cooldownState == CD_IDLE)
	{
		cooldownState = CD_PRECOOLDOWN;
		SetGlowLights(false);
		cooldownTime = gameLocal.time + COOLDOWN_PRECOOLDOWNDELAY;		
		BecomeActive(TH_THINK);
	}	
}

