//#include "sys/platform.h"
//#include "Entity.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "Item.h"
#include "Player.h"
#include "framework/Common.h"
#include "idlib/LangDict.h"
#include "bc_meta.h"
#include "bc_pa_control.h"

#define ENEMY_PROXIMITY_RADIUS 256
#define MAX_INTERACTION_DISTANCE 120 //if player gets too far, then interrupt the message. Note: make sure this value is larger than player.cpp FROB_DISTANCE

#define DOTPRODUCT_THRESHOLD .75f //how much can the player turn away from the mic before it disconnects

const idVec3 IDLECOLOR = idVec3(0, 1, 0);
const idVec3 SPEAKINGCOLOR = idVec3(1, .9f, 0);

CLASS_DECLARATION(idStaticEntity, idPA_Control)
END_CLASS

idPA_Control::idPA_Control(void)
{
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);

	needsRepair = false;
	repairrequestTimestamp = 0;

	idleSmoke = nullptr;
	flagModel = nullptr;

	flagState = FLG_INACTIVE;
	flagCheckTimer = 0;

	state = 0;
	stateTimer = 0;

	soundwaveEmitter = nullptr;
}

idPA_Control::~idPA_Control(void)
{
	repairNode.Remove();
}

void idPA_Control::Spawn(void)
{
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	isFrobbable = true;
	state = PA_NONE;

	SetColor(IDLECOLOR);
	UpdateVisuals();

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.


	//Spawn flag.
	idAngles flagAngle = GetPhysics()->GetAxis().ToAngles();
	//flagAngle.pitch = 0;
	//flagAngle.roll = 0;

	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idVec3 flagOffset = spawnArgs.GetVector("flag_offset");
	idVec3 flagPos = GetPhysics()->GetOrigin() + (forward * flagOffset.x) + (right * flagOffset.y) + (up * flagOffset.z);

	idDict args;
	args.Clear();
	args.SetVector("origin", flagPos);
	args.SetFloat("angle", flagAngle.yaw);
	args.SetBool("hide", true);
	args.Set("model", spawnArgs.GetString("model_flag"));
	flagModel = (idAnimatedEntity*)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);
	
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_soundwave"));
	args.Set("start_off", "1");
	soundwaveEmitter = static_cast<idFuncEmitter*>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	
	


	BecomeActive( TH_THINK );
}

void idPA_Control::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke
	savefile->WriteObject( flagModel ); // idAnimatedEntity* flagModel
	savefile->WriteInt( flagCheckTimer ); // int flagCheckTimer
	savefile->WriteInt( flagState ); // int flagState

	savefile->WriteObject( soundwaveEmitter ); // idFuncEmitter* soundwaveEmitter
}

void idPA_Control::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke
	savefile->ReadObject( CastClassPtrRef(flagModel) ); // idAnimatedEntity* flagModel
	savefile->ReadInt( flagCheckTimer ); // int flagCheckTimer
	savefile->ReadInt( flagState ); // int flagState

	savefile->ReadObject( CastClassPtrRef(soundwaveEmitter) ); // idFuncEmitter* soundwaveEmitter
}

void idPA_Control::Think(void)
{	
	idStaticEntity::Think();

	if (state == PA_PLAYERSPEAKING)
	{
		//Do distance check.
		float dist = (GetMicPosition() - gameLocal.GetLocalPlayer()->GetEyePosition()).Length();
		if (dist > MAX_INTERACTION_DISTANCE)
		{
			gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_mic_toofar");

			InterruptAllclear();
			return;
		}

		//Do dotproduct check.
		idVec3 dirToTarget =  GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		dirToTarget.Normalize();
		float vdot = DotProduct(dirToTarget, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
		if (vdot < DOTPRODUCT_THRESHOLD)
		{
			gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_mic_turn");

			InterruptAllclear();
			return;
		}


		//Update sound wave.
		if (!soundwaveEmitter->GetParticleActive())
			soundwaveEmitter->SetActive(true);

		idVec3 playerMouthPos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin + idVec3(0, 0, -2);
		soundwaveEmitter->SetOrigin(playerMouthPos);
		idVec3 micPosition = GetMicPosition() + idVec3(0, 0, 2); //for some reason it looks better if the soundwave floats toward a point above the mic
		idAngles waveAngles = (micPosition - playerMouthPos).ToAngles();
		waveAngles.pitch += 90;
		soundwaveEmitter->SetAxis(waveAngles.ToMat3());


		if (gameLocal.time > stateTimer)
		{
			//All-clear successful.
			state = PA_NONE;
			isFrobbable = true;

			SetColor(IDLECOLOR);
			UpdateVisuals();

			Event_GuiNamedEvent(1, "blink");
			Event_SetGuiParm("guitext", "#str_def_gameplay_pa_successful"); /*All-clear successful*/

			if (soundwaveEmitter->GetParticleActive())
				soundwaveEmitter->SetActive(false);

		}




	}

	if (health > 0 && gameLocal.time > flagCheckTimer)
	{
		flagCheckTimer = gameLocal.time + 300;

		if (flagState == FLG_INACTIVE)
		{
			if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT)
			{
				flagModel->Show();
				flagState = FLG_ACTIVATING;
				flagModel->Event_PlayAnim("deploy", 0);
			}
		}
		else if (flagState == FLG_ACTIVATING)
		{
			if (gameLocal.time > 1500)
			{
				flagState = FLG_ACTIVE;				
				flagModel->Event_PlayAnim("active", 1, true);
			}
		}
		else if (flagState == FLG_ACTIVE)
		{
			if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_IDLE)
			{
				flagState = FLG_INACTIVE;
				flagModel->Event_PlayAnim("undeploy", 1);
			}
		}
	}
}



void idPA_Control::DoRepairTick(int amount)
{
	//Repair is done!
	health = maxHealth;
	needsRepair = false;
	fl.takedamage = true;
	SetColor(IDLECOLOR);
	state = PA_NONE;

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}
}

void idPA_Control::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	needsRepair = true;
	repairrequestTimestamp = gameLocal.time;
	SetColor(0, 0, 0);
	state = PA_NONE;

	if (idleSmoke == NULL)
	{
		idDict args;
		args.Clear();
		args.Set("model", spawnArgs.GetString("model_damagesmoke"));
		args.Set("start_off", "0");
		idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
		idleSmoke->SetOrigin(GetMicPosition());
		idAngles angles = GetPhysics()->GetAxis().ToAngles();
		angles.pitch += 90;
		idleSmoke->SetAxis(angles.ToMat3());
	}
	else
	{
		idleSmoke->SetActive(true);
	}
}

bool idPA_Control::DoFrob(int index, idEntity * frobber)
{
	if (health <= 0)
	{
		StartSound("snd_cancel", SND_CHANNEL_BODY);
		return true;
	}

	if (state == PA_PLAYERSPEAKING)
		return true;

	if (frobber == NULL)
		return true;

	if (frobber->entityNumber != gameLocal.GetLocalPlayer()->entityNumber) //only allow player to frob.
	{
		StartSound("snd_static", SND_CHANNEL_BODY);
		return true;
	}

	int worldState = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState();
	if (worldState == COMBATSTATE_IDLE)
	{
		StartSound("snd_cancel", SND_CHANNEL_BODY);

		Event_GuiNamedEvent(1, "shake");
		Event_SetGuiParm("guitext", "#str_def_gameplay_pa_errorinactive"); /*Combat alert currently not active*/

		return true;
	}

	if (IsEnemyNear())
	{
		StartSound("snd_cancel", SND_CHANNEL_BODY);

		Event_GuiNamedEvent(1, "shake");
		Event_SetGuiParm("guitext", "#str_def_gameplay_pa_errornearby");/*Cannot end search when enemies are nearby*/

		return true;
	}

	//Player frobbed it.
	isFrobbable = false;
	int len;
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartPlayerWalkietalkieSequence(&len);
	
	//gameLocal.GetLocalPlayer()->StartSound(gameLocal.GetLocalPlayer()->GetInjured() ? "snd_vo_walkie_hijack_injured" : "snd_allclear", SND_CHANNEL_VOICE, 0, false, &len);
	stateTimer = gameLocal.time + len + ALLCLEAR_GAP_DURATION;
	state = PA_PLAYERSPEAKING;

	//gameLocal.DoParticle(spawnArgs.GetString("model_frobparticle"), GetMicPosition());

	SetColor(SPEAKINGCOLOR);
	UpdateVisuals();

	Event_GuiNamedEvent(1, "blink");
	Event_SetGuiParm("guitext", "#str_def_gameplay_pa_transmitting");/*Transmitting...*/



	return true;
}

bool idPA_Control::IsEnemyNear()
{
	for (idEntity *ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
	{
		if (!ent)
			continue;

		if (ent->health <= 0 || ent->IsHidden() || !ent->IsType(idAI::Type) || ent->team != TEAM_ENEMY)
			continue;

		if (!static_cast<idAI *>(ent)->CanAcceptStimulus())
			continue;

		float distance = (ent->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).Length();
		if (distance < ENEMY_PROXIMITY_RADIUS)
			return true;
	}

	return false;
}

void idPA_Control::InterruptAllclear()
{
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetAllClearInterrupt();
	state = PA_NONE;
	StartSound("snd_interrupt", SND_CHANNEL_BODY);
	
	
	Event_GuiNamedEvent(1, "shake");
	Event_SetGuiParm("guitext", "#str_def_gameplay_pa_errorinterrupt");/*Transmission interrupted*/

	isFrobbable = true;

	SetColor(IDLECOLOR);
	UpdateVisuals();

	if (soundwaveEmitter->GetParticleActive())
		soundwaveEmitter->SetActive(false);

	gameLocal.DoParticle(spawnArgs.GetString("model_fail"), GetMicPosition());
}

idVec3 idPA_Control::GetMicPosition()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idVec3 micOffset = spawnArgs.GetVector("mic_offset");
	return GetPhysics()->GetOrigin() + (forward * micOffset[0]) + (right * micOffset[1]) + (up * micOffset[2]);
}