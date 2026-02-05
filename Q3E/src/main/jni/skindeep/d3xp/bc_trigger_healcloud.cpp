#include "Trigger.h"
#include "framework/DeclEntityDef.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

#include "bc_trigger_healcloud.h"

CLASS_DECLARATION(idTrigger_Multi, idTrigger_healcloud)
EVENT(EV_Touch, idTrigger_healcloud::Event_Touch)
END_CLASS

idTrigger_healcloud::idTrigger_healcloud()
{
}

idTrigger_healcloud::~idTrigger_healcloud(void)
{
}

void idTrigger_healcloud::Spawn()
{
	active = true;
	fl.takedamage = true;
	idTrigger_Multi::Spawn();

	maxlifetime = gameLocal.time + spawnArgs.GetInt("spewLifetime");

	//Particle fx.
	particleEmitter = NULL;
	idDict splashArgs;
	splashArgs.Set("model", spawnArgs.GetString("spewParticle", "healcloud01.prt"));
	splashArgs.Set("start_off", "1");
	splashArgs.SetVector("origin", this->GetPhysics()->GetOrigin());
	particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));
	particleEmitter->SetActive(true);
}

void idTrigger_healcloud::Save(idSaveGame* savefile) const
{
	savefile->WriteBool( active ); // bool active
	savefile->WriteObject( particleEmitter ); // idFuncEmitter * particleEmitter
	savefile->WriteInt( maxlifetime ); // int maxlifetime
}
void idTrigger_healcloud::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool( active ); // bool active
	savefile->ReadObject( CastClassPtrRef(particleEmitter) ); // idFuncEmitter * particleEmitter
	savefile->ReadInt( maxlifetime ); // int maxlifetime
}


void idTrigger_healcloud::Event_Touch(idEntity* other, trace_t* trace)
{
	if (other->IsType(idActor::Type))
	{
		if (other == gameLocal.GetLocalPlayer())
		{
			//player touched it.
			//int maxhealth = gameLocal.GetLocalPlayer()->maxHealth;

            //Remove any player wound ailments.
            gameLocal.GetLocalPlayer()->HealAllWounds();


			if (!gameLocal.GetLocalPlayer()->GiveHealthAdjustedMax())
			{
				//player can't accept health right now. exit here.
				return;
			}
		}
		else
		{
			//is an actor. (but is not player)

			if (other->health >= other->maxHealth || other->health <= 0)
			{
				return;
			}

			other->health = other->maxHealth; //give health to actor.
		}

		idStr logStr = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_heal_breathed"), common->GetLanguageDict()->GetString(other->displayName));
		gameLocal.AddEventLog(logStr.c_str(), GetPhysics()->GetOrigin(), true, EL_HEAL);

		//ok, SOMEONE, either the player or an actor, breathed in the cloud. So, remove the cloud now.
		idEntityFx::StartFx("fx/healcloud_use", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		active = false;

		if (particleEmitter)
		{			
			particleEmitter->SetActive(false);
			particleEmitter->PostEventMS(&EV_Remove, 2000); //let the particle linger a bit so that it disappears during the middle of the fireball.
			particleEmitter = nullptr;
		}

		//Remove self.
		this->PostEventMS(&EV_Remove, 0);
	}
}



void idTrigger_healcloud::Think()
{
	if (!active)
		return;

	if (gameLocal.time > maxlifetime)
	{
		//Expired, make it go away.
		particleEmitter->SetActive(false);
		particleEmitter->PostEventMS(&EV_Remove, 2000);
		particleEmitter = nullptr;
		this->PostEventMS(&EV_Remove, 0);
	}
}
