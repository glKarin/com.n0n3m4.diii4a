#include "Trigger.h"
#include "Player.h"
#include "ai/AI.h"

#include "bc_trigger_sneeze.h"

//Sneeze trigger. Fires off every 300 ms.

const int AI_TOUCH_DESPAWNDELAY = 1500; //after AI touches it, make it despawn after XXX ms. We despawn it so that the AI doesn't just get stuck in the same cloud for a long time. But we want the cloud to visually hang around for at least a short time.
const int DESPAWN_PARTICLETIME = 3000; //how long it takes for particles to despawn. This is dependent on the particle durations in the .prt file.

const int UPDATETIME = 300;

CLASS_DECLARATION(idTrigger_Multi, idTrigger_sneeze)
EVENT(EV_Touch, idTrigger_sneeze::Event_Touch)
END_CLASS

idTrigger_sneeze::idTrigger_sneeze()
{
}

void idTrigger_sneeze::Spawn()
{
	idTrigger_Multi::Spawn();
	lastUpdatetime = 0;	

	multiplier = spawnArgs.GetFloat("multiplier", "1");
	maxlifetime = gameLocal.time + spawnArgs.GetInt("spewLifetime");

	//Particle fx.
	particleEmitter = NULL;
	idDict splashArgs;
	splashArgs.Set("model", spawnArgs.GetString("spewParticle", "pepperburst01.prt"));
	splashArgs.Set("start_off", "1");
	splashArgs.SetVector("origin", this->GetPhysics()->GetOrigin());
	particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));
	particleEmitter->SetActive(true);

	active = true;
	touchedByAI = false;
}


void idTrigger_sneeze::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( lastUpdatetime ); // int lastUpdatetime
	savefile->WriteFloat( multiplier ); // float multiplier

	savefile->WriteObject( particleEmitter ); // idFuncEmitter * particleEmitter

	savefile->WriteInt( maxlifetime ); // int maxlifetime

	savefile->WriteBool( active ); // bool active
	savefile->WriteBool( touchedByAI ); // bool touchedByAI
}
void idTrigger_sneeze::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( lastUpdatetime ); // int lastUpdatetime
	savefile->ReadFloat( multiplier ); // float multiplier

	savefile->ReadObject( CastClassPtrRef(particleEmitter) ); // idFuncEmitter * particleEmitter

	savefile->ReadInt( maxlifetime ); // int maxlifetime

	savefile->ReadBool( active ); // bool active
	savefile->ReadBool( touchedByAI ); // bool touchedByAI
}


void idTrigger_sneeze::Event_Touch(idEntity* other, trace_t* trace)
{
	if (other->IsType(idAI::Type) && static_cast<idAI*>(other)->team == TEAM_ENEMY)
	{
		//do the special Stun Damage.
		static_cast<idAI *>(other)->StartStunState("damage_sneeze");

		if (!touchedByAI)
		{
			touchedByAI = true;
			maxlifetime = min(gameLocal.time + AI_TOUCH_DESPAWNDELAY, maxlifetime);
		}
	}

	if (!other->IsType(idPlayer::Type))
		return;

	if (lastUpdatetime > gameLocal.time)
		return;

	lastUpdatetime = gameLocal.time + UPDATETIME;
	
	((idPlayer *)other)->SetSneezeDelta(false, multiplier);
}

void idTrigger_sneeze::Think()
{
	if (gameLocal.time > maxlifetime)
	{
		Despawn();
	}
}

void idTrigger_sneeze::Despawn()
{
	if (!active)
		return;

	active = false;
	particleEmitter->SetActive(false);
	particleEmitter->PostEventMS(&EV_Remove, DESPAWN_PARTICLETIME);
	particleEmitter = nullptr;
	this->PostEventMS(&EV_Remove, 0);	
}