#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
//#include "bc_ftl.h"

#include "bc_hazardpipe.h"
#include "bc_electricalbox.h"
#include "bc_turret.h"
#include "bc_meta.h"
#include "SecurityCamera.h"

const int REPAIR_DELAYTIME = 20000; //delay before I am repaired. Allow things to be broken for a while before repairing it.

#define DARKRED_LIGHTCOLOR idVec4(.2f, 0, 0, 1)

#define INTERESTDELAY_DURATION 500

CLASS_DECLARATION(idStaticEntity, idElectricalBox)
	//EVENT(EV_Activate, idElectricalBox::Event_Activate)
	EVENT(EV_PostSpawn, idElectricalBox::Event_PostSpawn)
END_CLASS

idElectricalBox::idElectricalBox(void)
{
	state = IDLE;
	idleSmoke = nullptr;
	hazardType = 0;

	interestDelayActive = false;
	interestDelayTimer = 0;

	electricalboxNode.SetOwner(this);
	electricalboxNode.AddToEnd(gameLocal.electricalboxEntities);

	//make it repairable.
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idElectricalBox::~idElectricalBox(void)
{
	electricalboxNode.Remove();
}

void idElectricalBox::Spawn(void)
{
	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);	

	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	idleSmoke = NULL;

	state = IDLE;
	hazardType = PIPETYPE_ELECTRICAL;

	isFrobbable = false;

	PostEventMS(&EV_PostSpawn, 0);
}

void idElectricalBox::Event_PostSpawn(void)
{
	idVec3 boxForward;
	idLocationEntity *locationEntity = NULL;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&boxForward, NULL, NULL);
	locationEntity = gameLocal.LocationForPoint(this->GetPhysics()->GetOrigin() + boxForward * 4);

	if (locationEntity == NULL)
	{
		gameLocal.Error("electricalbox '%s' cannot find location entity in the room.", this->GetName());
	}

	//Find entities associated with me.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;		

		idLocationEntity *currentLoc = NULL;

		//Gather up the lights.
		if (gameLocal.entities[i]->IsType(idLight::Type))
		{
			if (!gameLocal.entities[i]->spawnArgs.GetBool("affectemp", "1")) //skip the light if it has a special flag.
				continue;

			if (static_cast<idLight *>(gameLocal.entities[i])->IsFog()) //ignore fog lights.
				continue;

			//Lights are a special case. We want to get the light_center location.

			idStr modelname = gameLocal.entities[i]->spawnArgs.GetString("model");
			if (modelname.Length() <= 0)
			{
				//This is a light entity, not a light fixture.
				// SW 3rd April 2025: Need to rotate light_center around the light's axis, or else we might think it's in a wall when it isn't
				idVec3 lightCenter = gameLocal.entities[i]->spawnArgs.GetVector("light_center", "0 0 0");
				idMat3 lightAxis = gameLocal.entities[i]->GetPhysics()->GetAxis();
				currentLoc = gameLocal.LocationForPoint(gameLocal.entities[i]->GetPhysics()->GetOrigin() + (lightCenter * lightAxis));
			}
			else
			{
				//This is a light fixture.
				currentLoc = gameLocal.LocationForEntity(gameLocal.entities[i]);
			}
		}
		else
		{
			currentLoc = gameLocal.LocationForEntity(gameLocal.entities[i]);
			//currentLoc = gameLocal.LocationForPoint(gameLocal.entities[i]->GetPhysics()->GetOrigin());
		}


		if (currentLoc == NULL)
			continue;

		//If not in this room, then skip it.
		if (currentLoc->entityNumber != locationEntity->entityNumber)
			continue;

		//Associate electrical pipes & light fixtures with electrical box in the room.
		if (hazardType == PIPETYPE_ELECTRICAL)
		{
			//Found pipe(s) associated with this box.
			if (gameLocal.entities[i]->IsType(idHazardPipe::Type) && gameLocal.entities[i]->spawnArgs.GetInt("hazardtype") == hazardType)
			{
				static_cast<idHazardPipe *>(gameLocal.entities[i])->SetControlbox(this);
				pipeIndexes.Append(i);
			}

			//Find lights in this room.
			if (gameLocal.entities[i]->IsType(idLight::Type) && gameLocal.entities[i]->GetBindMaster() == NULL)
			{
				//gameRenderWorld->DebugArrowSimple(gameLocal.entities[i]->GetPhysics()->GetOrigin());
				//gameRenderWorld->DebugTextSimple(gameLocal.entities[i]->GetName(), gameLocal.entities[i]->GetPhysics()->GetOrigin());

				lightIndexes.Append(i);
			}

			//Find turrets in this room.
			if (gameLocal.entities[i]->IsType(idTurret::Type))
			{
				turretIndexes.Append(i);
			}

			//Find security cameras in this room.
			if (gameLocal.entities[i]->IsType(idSecurityCamera::Type))
			{
				securitycameraIndexes.Append(i);
			}
		}
	}

	


	//BecomeActive(TH_THINK);
}

void idElectricalBox::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state

	savefile->WriteObject( idleSmoke ); //  idFuncEmitter			* idleSmoke

	SaveFileWriteArray(pipeIndexes, pipeIndexes.Num(), WriteInt);
	SaveFileWriteArray(lightIndexes, lightIndexes.Num(), WriteInt);
	SaveFileWriteArray(turretIndexes, turretIndexes.Num(), WriteInt);
	SaveFileWriteArray(securitycameraIndexes, securitycameraIndexes.Num(), WriteInt);

	savefile->WriteInt( hazardType ); //  int hazardType

	savefile->WriteBool( interestDelayActive ); //  bool interestDelayActive
	savefile->WriteInt( interestDelayTimer ); //  int interestDelayTimer
}

void idElectricalBox::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); //  idFuncEmitter * idleSmoke

	SaveFileReadList(pipeIndexes, ReadInt); //  idList<int> pipeIndexes
	SaveFileReadList(lightIndexes, ReadInt); //  idList<int> lightIndexes
	SaveFileReadList(turretIndexes, ReadInt); //  idList<int> turretIndexes
	SaveFileReadList(securitycameraIndexes, ReadInt); //  idList<int> securitycameraIndexes

	savefile->ReadInt( hazardType ); //  int hazardType

	savefile->ReadBool( interestDelayActive ); //  bool interestDelayActive
	savefile->ReadInt( interestDelayTimer ); //  int interestDelayTimer
}

void idElectricalBox::Think(void)
{
	if (gameLocal.time > interestDelayTimer && interestDelayActive && state == DEAD)
	{
		interestDelayActive = false;

		idVec3 forward;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin() + (forward * 4), spawnArgs.GetString("def_deathinterest"));

		BecomeInactive(TH_THINK);
	}

	idStaticEntity::Think();
}

bool idElectricalBox::Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location)
{
	idStr particlename = spawnArgs.GetString("model_damage");
	if (particlename.Length() > 0)
	{
		idVec3 forward;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		idVec3 particlePos = GetPhysics()->GetOrigin() + (forward * 3.5f);

		idAngles particleAng = idAngles(GetPhysics()->GetAxis().ToAngles().pitch - 90, GetPhysics()->GetAxis().ToAngles().yaw, 0);
		gameLocal.DoParticle(particlename.c_str(), particlePos, particleAng.ToForward());
	}
	
	StartSound("snd_damage", SND_CHANNEL_ANY);

	return true;
}

void idElectricalBox::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (health > 0)
	{
		//electrical damage does crazy damage.
		float  _dmgMultiplier = damageScale;
		const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
		if (damageDef)
		{
			if (damageDef->GetBool("iselectrical"))
			{		
				_dmgMultiplier = 500;
			}
		}

		idStaticEntity::Damage(inflictor, attacker, dir, damageDefName, _dmgMultiplier, location, materialType);
	}
	
	//Death.
	if (health <= 0 && state == IDLE)
	{
		gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, damageDefName, EL_DESTROYED);

		state = DEAD;
		isFrobbable = false;
		StartSound("snd_broken", SND_CHANNEL_BODY, 0, false, NULL);

		StopSound(SND_CHANNEL_BODY2, false);
		StartSound("snd_shutdown", SND_CHANNEL_BODY2, 0, false, NULL);

		if (hazardType == PIPETYPE_ELECTRICAL)
		{
			idDict args;
			gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());			
			if (idleSmoke == NULL)
			{
				idVec3 forward;
				this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
				args.Clear();
				args.Set("model", "machine_damaged_smokeheavy4.prt");
				args.Set("start_off", "0");
				idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
				idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + (forward * 8));
			}
			else
			{
				idleSmoke->SetActive(true);
			}

			needsRepair = true;
			repairrequestTimestamp = gameLocal.time + REPAIR_DELAYTIME;

			//All the lights become emergency lighting.
			for (int i = 0; i < lightIndexes.Num(); i++)
			{
				int entNum = lightIndexes[i];

				if (!gameLocal.entities[entNum] || !gameLocal.entities[entNum]->IsType(idLight::Type))
					continue;

				//fade the light to red
				static_cast<idLight *>(gameLocal.entities[entNum])->Fade(DARKRED_LIGHTCOLOR, 0.5f);
				static_cast<idLight *>(gameLocal.entities[entNum])->SetAffectLightmeter(false); //light is fictionally turned off, so make light not affect lightmeter.
			}

			//Turn off the turrets.
			for (int i = 0; i < turretIndexes.Num(); i++)
			{
				int entNum = turretIndexes[i];
				if (!gameLocal.entities[entNum] || !gameLocal.entities[entNum]->IsType(idTurret::Type))
					continue;

				static_cast<idTurret *>(gameLocal.entities[entNum])->SetElectricalActive(false);
				static_cast<idTurret *>(gameLocal.entities[entNum])->Event_activate(false);
			}

			//Kill the security cameras.
			for (int i = 0; i < securitycameraIndexes.Num(); i++)
			{
				int entNum = securitycameraIndexes[i];
				if (!gameLocal.entities[entNum] || !gameLocal.entities[entNum]->IsType(idSecurityCamera::Type))
					continue;

				static_cast<idSecurityCamera *>(gameLocal.entities[entNum])->SetElectricalActive(false);
			}
		}

		interestDelayActive = true;
		interestDelayTimer = gameLocal.time + INTERESTDELAY_DURATION;
		BecomeActive(TH_THINK);
	}
}

void idElectricalBox::DoRepairTick(int amount)
{
	//Repair is done!

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}
	
	health = maxHealth;
	needsRepair = false;
	state = IDLE;
	StopSound(SND_CHANNEL_BODY2, false);
	StartSound("snd_poweron", SND_CHANNEL_BODY2, 0, false, NULL);
	//BecomeActive(TH_THINK);

	//iterate over all the pipes and reset them.
	for (int i = 0; i < pipeIndexes.Num(); i++)
	{
		int entNum = pipeIndexes[i];

		if (!gameLocal.entities[entNum]->IsType(idHazardPipe::Type))
			continue;

		static_cast<idHazardPipe *>(gameLocal.entities[entNum])->ResetPipe();
	}

	//Turn the lights back on.
	for (int i = 0; i < lightIndexes.Num(); i++)
	{
		int entNum = lightIndexes[i];

		if (!gameLocal.entities[entNum]->IsType(idLight::Type))
			continue;

		//fade the light to original color.
		idVec4 originalLightColor = static_cast<idLight *>(gameLocal.entities[entNum])->GetOriginalColor();
		static_cast<idLight *>(gameLocal.entities[entNum])->Fade(originalLightColor, 0.5f);

		//Make it affect the light meter (if applicable. Example: fog does not affect light meter.)
		bool originalLightmeterValue = static_cast<idLight *>(gameLocal.entities[entNum])->GetOriginalLightmeter();
		static_cast<idLight *>(gameLocal.entities[entNum])->SetAffectLightmeter(originalLightmeterValue);		
	}

	//Turn the turrets back on.
	for (int i = 0; i < turretIndexes.Num(); i++)
	{
		int entNum = turretIndexes[i];
		if (!gameLocal.entities[entNum]->IsType(idTurret::Type))
			continue;

		static_cast<idTurret *>(gameLocal.entities[entNum])->SetElectricalActive(true);
		if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate != COMBATSTATE_IDLE)
		{
			//if in combat state, then deploy the turret.
			static_cast<idTurret *>(gameLocal.entities[entNum])->Event_activate(true);
		}
	}
	

	//Turn the securitycameras back on.
	for (int i = 0; i < securitycameraIndexes.Num(); i++)
	{
		int entNum = securitycameraIndexes[i];
		if (!gameLocal.entities[entNum]->IsType(idSecurityCamera::Type))
			continue;

		static_cast<idSecurityCamera *>(gameLocal.entities[entNum])->SetElectricalActive(true);
	}


	//isFrobbable = true;
}



bool idElectricalBox::IsElectricalboxAlive()
{
	return (state == IDLE);
}