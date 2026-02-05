//#include "sys/platform.h"
//#include "Entity.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "Item.h"
#include "Player.h"
#include "bc_frobcube.h"
#include "bc_ftl_charger.h"

#define CHARGETIME 5000

#define POSTMESSAGETIME 5000

#define MAXDISTANCE 90

CLASS_DECLARATION(idStaticEntity, idFTLCharger)
END_CLASS

idFTLCharger::idFTLCharger(void)
{
	state = FC_NONE;
}

idFTLCharger::~idFTLCharger(void)
{
	if (particleEmitter != nullptr)
		delete particleEmitter;
}

void idFTLCharger::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	fl.takedamage = true;
	Event_SetGuiInt("chargemetervisible", 0);
	BecomeActive(TH_THINK);
	
	idDict args;
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_chargeparticle"));
	args.SetVector("origin", GetEmitterOrigin());
	args.SetBool("start_off", true);
	particleEmitter = static_cast<idFuncEmitter*>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
}



void idFTLCharger::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteObject( particleEmitter ); // idFuncEmitter* particleEmitter
}

void idFTLCharger::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadObject( CastClassPtrRef(particleEmitter) ); // idFuncEmitter* particleEmitter
}

void idFTLCharger::Think(void)
{	
	if (state == FC_CHARGING)
	{
		//Update charge progress meter.
		float lerp = (gameLocal.time - stateTimer) / (float)CHARGETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		Event_SetGuiFloat("chargeprogress", lerp);


		//Update emitter angle.
		idAngles dirToPlayer = ((gameLocal.GetLocalPlayer()->firstPersonViewOrigin + idVec3(0, 0, -8)) - GetEmitterOrigin()).ToAngles();
		dirToPlayer.pitch += 90;
		particleEmitter->SetAxis(dirToPlayer.ToMat3());

		
		//distance check for player.
		float distance = (gameLocal.GetLocalPlayer()->firstPersonViewOrigin - GetPhysics()->GetOrigin()).Length();
		if (distance > MAXDISTANCE)
		{
			//Player is too far.
			StopCharge();
			StartSound("snd_cancel", SND_CHANNEL_BODY2);
			Event_SetGuiParm("chargetext", "#str_def_gameplay_defibcharge_interrupted");
		}

		if (gameLocal.time > stateTimer + CHARGETIME)
		{
			//Done charging.
			StopCharge();			
			StartSound("snd_success", SND_CHANNEL_BODY2);
			Event_SetGuiParm("chargetext", "#str_def_gameplay_defibcharge_done");
			gameLocal.GetLocalPlayer()->SetDefibAvailable(true);
		}
	}
	else if (state == FC_POSTMESSAGE)
	{
		if (gameLocal.time > stateTimer + POSTMESSAGETIME)
		{
			state = FC_NONE;

			//change the text.
			Event_SetGuiParm("chargetext", "#str_def_gameplay_defibcharge_detected");
		}
	}

	idStaticEntity::Think();
}

idVec3 idFTLCharger::GetEmitterOrigin()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	return GetPhysics()->GetOrigin() + (forward * 2) + (right * 2.2f) + (up * -5.5f);
}

void idFTLCharger::StopCharge()
{
	stateTimer = gameLocal.time;
	state = FC_POSTMESSAGE;
	Event_SetGuiInt("chargemetervisible", 0);
	isFrobbable = true;
	particleEmitter->SetActive(false);
}

bool idFTLCharger::DoFrob(int index, idEntity* frobber)
{
	if (frobber == nullptr)
		return true;

	//see if auto defib needs to be charged.
	bool defibAvailable = gameLocal.GetLocalPlayer()->GetDefibAvailable();

	if (defibAvailable)
	{
		//Defib is already charged.
		stateTimer = gameLocal.time;
		state = FC_POSTMESSAGE;
		Event_SetGuiParm("chargetext", "#str_def_gameplay_defibcharge_already");
		StartSound("snd_beep", SND_CHANNEL_BODY2);
	}
	else
	{
		//Start charging.
		Event_SetGuiParm("chargetext", "#str_def_gameplay_defibcharge_charging");
		state = FC_CHARGING;
		Event_SetGuiInt("chargemetervisible", 1);
		stateTimer = gameLocal.time;
		Event_SetGuiFloat("chargeprogress", 0);
		isFrobbable = false;
		StartSound("snd_beep", SND_CHANNEL_BODY2);
		particleEmitter->SetActive(true);
	}
	

	return true;
}
