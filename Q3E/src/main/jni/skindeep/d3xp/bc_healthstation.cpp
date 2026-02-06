#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "bc_meta.h"
#include "bc_trigger_healcloud.h"
#include "bc_healthstation.h"

const int HEALCLOUD_FORWARDDISTANCE = 32;

const float UNLOCKDOWN_COLOR_G = .6f;
const float LOCKDOWN_COLOR_R = .6f;

const int DISPENSE_DELAY = 400;

#define HEALCLOUD_RADIUS "64"
#define HEALCLOUD_LIFETIME "4000"

#define LEVER_COOLDOWNTIME 1500

#define PROXIMITYANNOUNCE_CHECKTIME 1000
#define PROXIMITYANNOUNCE_ACTIVATIONRADIUS 256
#define PROXIMITYANNOUNCE_PLAYERCOOLDOWN 2000

#define BLOODDISPENSE_TIME 2400 //how long does it give blood
#define BLOODDISPENSE_TICKINTERVAL 20
#define BLOODDISPENSE_DISTANCE 112

const idEventDef EV_healthstation_setannouncer("healthstation_setannouncer", "d");

CLASS_DECLARATION(idAnimated, idHealthstation)
	EVENT(EV_healthstation_setannouncer, idHealthstation::Event_SetHealthstationAnnouncer)
END_CLASS


idHealthstation::idHealthstation(void)
{
	bloodDispenseTimer = 0;
	bloodDispenseTickInterval = 0;

	gameLocal.healthstationEntities.Append(this);

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	proximityAnnouncer.sensor = this;
	proximityAnnouncer.checkHealth = true;

	proximityannouncerActive = true;

	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idHealthstation::~idHealthstation(void)
{
	repairNode.Remove();

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idHealthstation::Spawn(void)
{
	isFrobbable = true;
	fl.takedamage = true;
	stateTimer = 0;
	state = HEALTHSTATION_IDLE;

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);


	

	//renderlight.
	if (1)
	{
		idVec3 forward;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

		// Light source.
		headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 12.0f;
		headlight.shaderParms[0] = 0;
		headlight.shaderParms[1] = .6f;
		headlight.shaderParms[2] = 0;
		headlight.shaderParms[3] = 1.0f;
		headlight.noShadows = true;
		headlight.isAmbient = false;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);

		headlight.origin = GetPhysics()->GetOrigin() + (forward * 6);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	SetCombatLockdown(false);
	SetColor(0, 1, 0); //green.

	doBloodDispense = false;
	playerWasLastFrobber = false;

	//repair system.
	needsRepair = false;
	repairrequestTimestamp = 0;

	proximityAnnouncer.Start();

	team = TEAM_NEUTRAL;

	BecomeActive(TH_THINK);

	PostEventMS(&EV_PostSpawn, 100);
}

//BC 2-26-2025: moved this to post spawn, so that we can guarantee idmeta exists in world at this point.
void idHealthstation::Event_PostSpawn(void)
{
	//Find a suitable space to spawn the idleTask.
	idVec3 forward, forwardPos;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	trace_t downTr;
	forwardPos = this->GetPhysics()->GetOrigin() + forward * 32;
	gameLocal.clip.TracePoint(downTr, forwardPos, forwardPos + idVec3(0, 0, -72), MASK_SOLID, NULL);
	if (downTr.fraction >= 1.0f)
	{
		common->Warning("healthstation %s: failed to find place for idletask.\n", GetName());
		return; //Wasn't able to find a piece of floor..... this should never happen.
	}

	idMeta* meta = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity());
	if (meta != NULL)
	{
		meta->SpawnIdleTask(this, downTr.endpos, "idletask_healthstation"); //spawn the idletask.
	}
	else
	{
		common->Warning("healthstation %s: metaEnt is NULL.\n", GetName());
	}
}


void idHealthstation::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteBool( doBloodDispense ); // bool doBloodDispense
	savefile->WriteBool( playerWasLastFrobber ); // bool playerWasLastFrobber
	savefile->WriteInt( bloodDispenseTimer ); // int bloodDispenseTimer
	savefile->WriteInt( bloodDispenseTickInterval ); // int bloodDispenseTickInterval

	proximityAnnouncer.Save(savefile); // idProximityAnnouncer proximityAnnouncer

	savefile->WriteBool( proximityannouncerActive ); // bool proximityannouncerActive
}

void idHealthstation::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state

	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadBool( doBloodDispense ); // bool doBloodDispense
	savefile->ReadBool( playerWasLastFrobber ); // bool playerWasLastFrobber
	savefile->ReadInt( bloodDispenseTimer ); // int bloodDispenseTimer
	savefile->ReadInt( bloodDispenseTickInterval ); // int bloodDispenseTickInterval

	proximityAnnouncer.Restore(savefile); // idProximityAnnouncer proximityAnnouncer

	savefile->ReadBool( proximityannouncerActive ); // bool proximityannouncerActive
}

void idHealthstation::Think(void)
{
	if (state == HEALTHSTATION_DISPENSEDELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			state = HEALTHSTATION_DISPENSING;
			stateTimer = gameLocal.time + LEVER_COOLDOWNTIME;

			//Check if we're doing the blood dispense. This only happens if player is bleeding out.
			if (doBloodDispense)
			{
				StartSound("snd_blood", SND_CHANNEL_VOICE2);
				StartSound("snd_squirt", SND_CHANNEL_BODY);
				proximityAnnouncer.DoSoundwaves();

				//gameLocal.GetLocalPlayer()->health = 110; //refill bleedout bar. Give it a little extra as a bonus.

				//Bloodstream particles from nozzle.
				if (g_bloodEffects.GetBool())
				{
					DoNozzleParticle("heal_bloodnozzle.prt", true);
				}
				else
				{
					// SW 18th Feb 2025: We don't want blood, 
					// but we do want the player to be able to see that the health station is dispensing something,
					// so we use a special bloodless particle
					DoNozzleParticle("heal_bloodnozzle_noblood.prt", true);
				}
				

				bloodDispenseTimer = gameLocal.time + BLOODDISPENSE_TIME;
				bloodDispenseTickInterval = 0;
			}
			else
			{
				DispenseHealcloud();
			}
		}
	}
	else if (state == HEALTHSTATION_DISPENSING)
	{
		//is spewing out healing gas

		if (gameLocal.time >= stateTimer)
		{
			//done spewing.

			state = HEALTHSTATION_IDLE;
			isFrobbable = true;
			Event_GuiNamedEvent(1, "dispenseEnd");
			SetColor(0, 1, 0); //green.
			UpdateVisuals();
			
			StartSound("snd_announcer", SND_CHANNEL_VOICE);
			proximityAnnouncer.DoSoundwaves();


			if (playerWasLastFrobber)
			{
				idVec3 forward, up;
				GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
				idVec3 interestPos = GetPhysics()->GetOrigin() + (forward * 2) + (up * 6.5f);
				gameLocal.SpawnInterestPoint(this, interestPos, spawnArgs.GetString("interest_use"));
			}
		}
	}
	else if (state == HEALTHSTATION_COMBATLOCKED)
	{
		//update the timer.
		//int timeRemaining = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetSearchTimer();
		//timeRemaining = timeRemaining - gameLocal.time;
		//timeRemaining = (int)(timeRemaining / (float)1000.0f);
		//
		//if (timeRemaining >= g_searchtime.GetInteger())
		//{
		//	Event_SetGuiParm("searchtimer", "");
		//}
		//else
		//{
		//	timeRemaining += 1;
		//	Event_SetGuiInt("searchtimer", timeRemaining);
		//}
	}
	else if (state == HEALTHSTATION_IDLE)
	{
		if (proximityannouncerActive)
		{
			proximityAnnouncer.Update();
		}		
	}
	

	if (bloodDispenseTimer > gameLocal.time)
	{
		//The machine is currently squirting out blood.

		if (gameLocal.GetLocalPlayer()->inDownedState && gameLocal.GetLocalPlayer()->health > 0 && gameLocal.time > bloodDispenseTickInterval)
		{
			bloodDispenseTickInterval = gameLocal.time + BLOODDISPENSE_TICKINTERVAL;

			float distanceToPlayer = (GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).Length();
			if (distanceToPlayer <= BLOODDISPENSE_DISTANCE)
			{				
				gameLocal.GetLocalPlayer()->GiveBleedoutHealth(1);
			}
		}
	}

	idAnimated::Think();
}

void idHealthstation::DispenseHealcloud()
{
	StartSound("snd_aerosol", SND_CHANNEL_BODY);

	//Spawn heal cloud.
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	idDict args;
	idTrigger_Multi* healTrigger;
	int radius = spawnArgs.GetInt("spewRadius", HEALCLOUD_RADIUS);
	args.Clear();
	args.SetVector("mins", idVec3(-radius, -radius, -radius));
	args.SetVector("maxs", idVec3(radius, radius, radius));
	args.SetVector("origin", GetPhysics()->GetOrigin() + forward * HEALCLOUD_FORWARDDISTANCE);
	args.Set("spewParticle", spawnArgs.GetString("spewParticle", "healcloud01.prt"));
	args.SetInt("spewLifetime", spawnArgs.GetInt("spewLifetime", HEALCLOUD_LIFETIME));
	healTrigger = (idTrigger_healcloud*)gameLocal.SpawnEntityType(idTrigger_healcloud::Type, &args);

	//Particle jet from nozzle.
	DoNozzleParticle("heal_jet.prt", false);

	if (gameLocal.GetLocalPlayer()->health >= gameLocal.GetLocalPlayer()->maxHealth && playerWasLastFrobber)
	{
		gameLocal.GetLocalPlayer()->ForceShowHealthbar(2000);
	}
}

bool idHealthstation::DoFrob(int index, idEntity * frobber)
{
	idEntity::DoFrob(index, frobber);

	if (state == HEALTHSTATION_DAMAGED)
	{
		//StartSound("snd_cancel", SND_CHANNEL_ANY);
		Event_PlayAnim("leverbroken", 1);
		return false;
	}

	if (state != HEALTHSTATION_IDLE || frobber == NULL)
	{
		Event_PlayAnim("leverbroken", 1);
		Event_GuiNamedEvent(1, "securityalert");
		return false;
	}

	playerWasLastFrobber = false;
	doBloodDispense = false;
	if (frobber != NULL)
	{
		if (frobber == gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->inDownedState)
		{
			doBloodDispense = true;
		}

		if (frobber == gameLocal.GetLocalPlayer())
		{
			playerWasLastFrobber = true;
		}
	}


	//if (gameLocal.time >= stateTimer)
	{
		StartSound("snd_dispense", SND_CHANNEL_BODY);

		state = HEALTHSTATION_DISPENSEDELAY;
		stateTimer = gameLocal.time + DISPENSE_DELAY;		
		isFrobbable = false;		

		Event_PlayAnim("activate", 1);
		SetColor(1, .7f, 0); //yellow.
		
		Event_GuiNamedEvent(1, "dispenseStart"); //Gui.

		// SW: for vignette scripting
		idStr scriptFunc = spawnArgs.GetString("callOnDispense", "");		
		if (scriptFunc.Length() > 0)
		{
			gameLocal.RunMapScript(scriptFunc);
		}
		
	}

	return true;
}

void idHealthstation::DoNozzleParticle(const char *particlename, bool pointTowardPlayer)
{
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	idAngles myAngle = GetPhysics()->GetAxis().ToAngles();
	myAngle.pitch += -90;
	
	if (pointTowardPlayer)
	{
		idVec3 angleToPlayer = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
		float yawToPlayer = angleToPlayer.ToYaw();
		myAngle.yaw = yawToPlayer + 8; //offset a little so the player can see the bloodstream arc.
	}

	idVec3 particlePos = GetPhysics()->GetOrigin() + (forward * 1) + (up * -6.5f);
	gameLocal.DoParticle(particlename, particlePos, myAngle.ToForward());
}

void idHealthstation::SetCombatLockdown(bool value)
{
	if (value)
	{
		//lock it down.
		SetColor(1, 0, 0);		 //red

		headlight.shaderParms[0] = LOCKDOWN_COLOR_R;
		headlight.shaderParms[1] = 0;
		headlight.shaderParms[2] = 0;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

		//if (state != HEALTHSTATION_COMBATLOCKED)
		//{
		//	Event_PlayAnim("leverhide", 1);
		//}
		state = HEALTHSTATION_COMBATLOCKED;
		this->Event_SetGuiInt("lockdown", 1);
	}
	else
	{
		if (state == HEALTHSTATION_IDLE || state == HEALTHSTATION_DAMAGED)
			return;

		UnlockMachine();
	}

	UpdateVisuals();
}

void idHealthstation::UnlockMachine()
{
	//unlock it.
	SetColor(0, 1, 0);				 //green

	headlight.shaderParms[0] = 0;
	headlight.shaderParms[1] = UNLOCKDOWN_COLOR_G;
	headlight.shaderParms[2] = 0;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

	//if (state != HEALTHSTATION_IDLE)
	//{
	//	Event_PlayAnim("levershow", 1);
	//}
	state = HEALTHSTATION_IDLE;
	this->Event_SetGuiInt("lockdown", 0);

	this->Event_GuiNamedEvent(1, "combatReset");
	isFrobbable = true;
}



void idHealthstation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (state == HEALTHSTATION_DAMAGED)
		return;

	const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
	if (!damageDef) {
		gameLocal.Error("Unknown damageDef '%s'\n", damageDefName);
	}

	int	damage = damageDef->GetInt("damage");

	if (damage > 0)
	{
		health -= damage;
	}

	if (health <= 0)
	{
		gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

		state = HEALTHSTATION_DAMAGED;
		//Event_PlayAnim("leverhide", 1);
		SetColor(idVec4(0, 0, 0, 0));
		needsRepair = true;
		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_broken")));
		this->Event_SetGuiInt("broken", 1);
		gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());
		isFrobbable = true;
		headlight.shaderParms[0] = 0;
		headlight.shaderParms[1] = 0;
		headlight.shaderParms[2] = 0;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

		StartSound("snd_explode", SND_CHANNEL_BODY);		

		StartSound("snd_break", SND_CHANNEL_VOICE);
		proximityAnnouncer.DoSoundwaves();
	}
}

void idHealthstation::DoRepairTick(int amount)
{
	//Repair is done!
	health = maxHealth;
	needsRepair = false;
	SetCombatLockdown(false);
	this->Event_SetGuiInt("broken", 0);
	SetSkin(declManager->FindSkin("skins/objects/healthstation/default"));
	UpdateVisuals();

	UnlockMachine();

}

idProximityAnnouncer::idProximityAnnouncer( idEntity* ent )
{
	sensor = ent;
	proximityCheckTimer = 0;
	coolDownTimer = 0;
	canProximityAnnounce = true;
	checkHealth = false;
	activationRadius = PROXIMITYANNOUNCE_ACTIVATIONRADIUS;
	checkPeriod = PROXIMITYANNOUNCE_CHECKTIME;
	coolDownPeriod = PROXIMITYANNOUNCE_PLAYERCOOLDOWN;
}

void idProximityAnnouncer::Start()
{
	canProximityAnnounce = true;
	static idRandom2 rand2 = idRandom2(0);
	static int announcerCount = 0;
	proximityCheckTimer = gameLocal.time + 1000 + announcerCount*100 + rand2.RandomInt(50);
	if(checkPeriod == 0) checkPeriod = 1;
}

void idProximityAnnouncer::Save(idSaveGame* savefile) const
{
	savefile->WriteObject( sensor ); // idEntity* sensor
	savefile->WriteInt( proximityCheckTimer ); // int proximityCheckTimer
	savefile->WriteBool( canProximityAnnounce ); // bool canProximityAnnounce
	savefile->WriteInt( coolDownTimer ); // int coolDownTimer
	savefile->WriteBool( checkHealth ); // bool checkHealth
	savefile->WriteInt( activationRadius ); // int activationRadius
	savefile->WriteInt( checkPeriod ); // int checkPeriod
	savefile->WriteInt( coolDownPeriod ); // int coolDownPeriod
}
void idProximityAnnouncer::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( sensor ); // idEntity* sensor
	savefile->ReadInt( proximityCheckTimer ); // int proximityCheckTimer
	savefile->ReadBool( canProximityAnnounce ); // bool canProximityAnnounce
	savefile->ReadInt( coolDownTimer ); // int coolDownTimer
	savefile->ReadBool( checkHealth ); // bool checkHealth
	savefile->ReadInt( activationRadius ); // int activationRadius
	savefile->ReadInt( checkPeriod ); // int checkPeriod
	savefile->ReadInt( coolDownPeriod ); // int coolDownPeriod
}

bool idProximityAnnouncer::Ready()
{
	return proximityCheckTimer <= gameLocal.time && coolDownTimer <= gameLocal.time;
}

void idProximityAnnouncer::Update()
{
	assert(sensor);
	assert(checkPeriod > 0);

	if (!Ready())
		return;

	// make sure the timer is caught up while still keeping the original cadence
	proximityCheckTimer = (1 + ((gameLocal.time - proximityCheckTimer)/checkPeriod)) * checkPeriod;

	//check if it's near someone.
	bool isNearSomeone = IsProximityNearSomeone();
	if (isNearSomeone && canProximityAnnounce)
	{
		DoSoundwaves();
		sensor->StartSound("snd_announce", SND_CHANNEL_VOICE);
		canProximityAnnounce = false;
		coolDownTimer = gameLocal.time + coolDownPeriod;
	}
	else if (!isNearSomeone && !canProximityAnnounce)
	{
		//if no one is near me, then I'm allowed to do the announcement again at the next suitable time.
		canProximityAnnounce = true;
	}
}

bool idProximityAnnouncer::IsProximityNearSomeone()
{
	assert(sensor);

	idVec3 loc = sensor->GetPhysics()->GetOrigin();
	idEntity	*ent;
	for (ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
	{
		if (!ent)
			continue;

		//if (ent->entityNumber == gameLocal.GetLocalPlayer()->entityNumber && coolDownTimer > gameLocal.time) //if player, then skip if we've recently done the announcement already		
		//	continue;

		if(checkHealth)
		{
			if (ent->health <= 0 || ent->IsHidden() || !ent->IsType(idActor::Type))
				continue;

			bool isPlayerSmelly = gameLocal.GetLocalPlayer()->GetSmelly() && ent->entityNumber == gameLocal.GetLocalPlayer()->entityNumber;
			bool downed = ent == gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->inDownedState;
			if (ent->health < ent->maxHealth || isPlayerSmelly || downed) //only do announcement if health is not maxed out, or player is smelly.
			{
				//valid, keep going.
			}
			else
			{
				//invalid, exit here.
				continue;
			}
		}

		// No physics??
		if (!ent->GetPhysics())
			continue;

		//do distance check.
		float distanceSqr = (loc - ent->GetPhysics()->GetOrigin()).LengthSqr();
		if (distanceSqr > activationRadius*activationRadius)
			continue; //too far.

		//do los check.
		trace_t eyeTr;
		gameLocal.clip.TracePoint(eyeTr, loc, static_cast<idActor *>(ent)->GetEyePosition(), MASK_SOLID, sensor);
		if (eyeTr.fraction >= 1.0f)
		{

			//if (ent->entityNumber == gameLocal.GetLocalPlayer()->entityNumber)
			//{
			//	//if player activates the announcement, then do a special cooldown for the player. To make the announcement less annoying.
			//	coolDownTimer = gameLocal.time + PROXIMITYANNOUNCE_PLAYERCOOLDOWN;
			//}


			return true;
		}
	}

	return false;
}

void idProximityAnnouncer::DoSoundwaves()
{
	assert(sensor);
	idVec3 forward, up;
	sensor->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	idVec3 soundPos = sensor->GetPhysics()->GetOrigin() + (forward * 2) + (up * 6.5f);

	idAngles particleAngle = sensor->GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch -= 90;
	gameLocal.DoParticle("sound_burst_small_directed.prt", soundPos, particleAngle.ToForward());
}


void idHealthstation::Event_SetHealthstationAnnouncer(int _value)
{
	proximityannouncerActive = _value > 0 ? true : false;
}

void idHealthstation::DoHack()
{
	DispenseHealcloud();

	SetCombatLockdown(false);
}