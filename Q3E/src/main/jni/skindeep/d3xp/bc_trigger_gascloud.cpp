#include "Trigger.h"
#include "Player.h"
#include "ai/AI.h"

#include "bc_trigger_gascloud.h"

//Gas trigger. From the engineer landmine. Makes you cough, eyes tear up.

const int AI_TOUCH_DESPAWNDELAY = 1500; //after AI touches it, make it despawn after XXX ms. We despawn it so that the AI doesn't just get stuck in the same cloud for a long time. But we want the cloud to visually hang around for at least a short time.
const int DESPAWN_PARTICLETIME = 3000; //how long it takes for particles to despawn. This is dependent on the particle durations in the .prt file.

const int UPDATETIME = 300;

CLASS_DECLARATION(idTrigger_Multi, idTrigger_gascloud)
EVENT(EV_Touch, idTrigger_gascloud::Event_Touch)
END_CLASS

idTrigger_gascloud::idTrigger_gascloud()
{
}

void idTrigger_gascloud::Spawn()
{
	idTrigger_Multi::Spawn();
	lastUpdatetime = 0;	

	maxlifetime = gameLocal.time + spawnArgs.GetInt("spewLifetime");

	//Particle fx.
	particleEmitter = NULL;
	idDict splashArgs;
	splashArgs.Set("model", spawnArgs.GetString("spewParticle", "gascloud01.prt"));
	splashArgs.Set("start_off", "1");
	splashArgs.SetVector("origin", this->GetPhysics()->GetOrigin());
	particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));
	particleEmitter->SetActive(true);

	active = true;
	touchedBySomeone = false;
}

void idTrigger_gascloud::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( lastUpdatetime ); // int lastUpdatetime
	savefile->WriteObject( particleEmitter ); // idFuncEmitter * particleEmitter
	savefile->WriteInt( maxlifetime ); // int maxlifetime

	savefile->WriteBool( active ); // bool active
	savefile->WriteBool( touchedBySomeone ); // bool touchedBySomeone
}
void idTrigger_gascloud::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( lastUpdatetime ); // int lastUpdatetime
	savefile->ReadObject( CastClassPtrRef(particleEmitter) ); // idFuncEmitter * particleEmitter
	savefile->ReadInt( maxlifetime ); // int maxlifetime

	savefile->ReadBool( active ); // bool active
	savefile->ReadBool( touchedBySomeone ); // bool touchedBySomeone
}


void idTrigger_gascloud::Event_Touch(idEntity* other, trace_t* trace)
{
	if (other->IsType(idAI::Type) && static_cast<idAI*>(other)->team == TEAM_ENEMY)
	{
		//do the special Stun Damage.
		static_cast<idAI *>(other)->StartStunState("damage_gascloud");

		// SW: adding support for calling a script when we stun something.
		// This should have been fed into the cloud by the entity spawning it
		idStr scriptCall;
		if (spawnArgs.GetString("callOnStun", "", scriptCall))
		{
			gameLocal.RunMapScriptArgs(scriptCall, other, this);
		}

		if (!touchedBySomeone)
		{
			//an AI has touched me. After a short delay, start the despawn sequence. We do a delay so the cloud doesn't just immediately disappear.
			touchedBySomeone = true;
			maxlifetime = min(gameLocal.time + AI_TOUCH_DESPAWNDELAY, maxlifetime);
		}
	}

	if (!other->IsType(idPlayer::Type))
		return;

	if (lastUpdatetime > gameLocal.time)
		return;

	lastUpdatetime = gameLocal.time + UPDATETIME;
	
	//PLAYER has touched the cloud.

	if (!((idPlayer *)other)->cond_gascloud)
	{
		((idPlayer *)other)->SetGascloudState(true);
	}
	
	if (!touchedBySomeone)
	{
		//Start despawning.
		touchedBySomeone = true;
		maxlifetime = min(gameLocal.time + AI_TOUCH_DESPAWNDELAY, maxlifetime);
	}
}

void idTrigger_gascloud::Think()
{
	if (gameLocal.time > maxlifetime)
	{
		Despawn();
	}
}

//Fade out the particle, delete self.
void idTrigger_gascloud::Despawn()
{
	if (!active)
		return;

	active = false;
	particleEmitter->SetActive(false);
	particleEmitter->PostEventMS(&EV_Remove, DESPAWN_PARTICLETIME);
	particleEmitter = nullptr;
	this->PostEventMS(&EV_Remove, 0);
}
